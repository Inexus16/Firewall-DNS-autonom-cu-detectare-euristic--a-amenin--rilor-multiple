#include "resolver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define MAX_RECURSION_DEPTH 16
#define DNS_TIMEOUT_SEC     3
#define ROOT_SERVERS_FILE   "root.hints"
#define EDNS_SIZE           1232

/* Internal nameserver list */
typedef struct {
    char   name[DNS_MAX_NAME_LEN+1];
    struct sockaddr_in addr;
} ns_record_t;

static ns_record_t *g_root_hints = NULL;
static size_t       g_root_count = 0;

/* Static forwarders for local test domains */
static struct {
    const char *zone;
    const char *ip;
    int         port;
} static_forwarders[] = {
    { "fastflux.test", "127.0.0.1", 5300 },
    { "tunnel.test",   "127.0.0.1", 5301 },
    { "signed.test",   "127.0.0.1", 5302 },
    { NULL, NULL, 0 }
};

// Persistent TCP connection pool
#define TCP_POOL_SIZE 4

typedef struct {
    int sock;
    pthread_mutex_t lock;
    int in_use;        /* flag, but lock already protects usage */
} tcp_conn_t;

static tcp_conn_t tcp_pool[TCP_POOL_SIZE];
static int pool_initialised = 0;
static pthread_mutex_t pool_lock = PTHREAD_MUTEX_INITIALIZER;
static char pool_fwd_ip[INET_ADDRSTRLEN];   /* forwarder IP used by the pool */

static void init_tcp_pool(const char *ip) {
    pthread_mutex_lock(&pool_lock);
    if (pool_initialised) {
        pthread_mutex_unlock(&pool_lock);
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        pthread_mutex_unlock(&pool_lock);
        return;
    }

    for (int i = 0; i < TCP_POOL_SIZE; i++) {
        tcp_pool[i].sock = -1;
        pthread_mutex_init(&tcp_pool[i].lock, NULL);
        tcp_pool[i].in_use = 0;

        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) continue;
        struct timeval tv = { DNS_TIMEOUT_SEC, 0 };
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(s);
            continue;
        }
        tcp_pool[i].sock = s;
    }

    strncpy(pool_fwd_ip, ip, INET_ADDRSTRLEN);
    pool_initialised = 1;
    pthread_mutex_unlock(&pool_lock);
}

/* borrow a connection; returns index or -1 on error. lock of that slot is held when returning. */
static int borrow_tcp_conn(void) {
    while (1) {
        for (int i = 0; i < TCP_POOL_SIZE; i++) {
            if (pthread_mutex_trylock(&tcp_pool[i].lock) == 0) {
                /* check if socket is healthy */
                if (tcp_pool[i].sock >= 0) {
                    tcp_pool[i].in_use = 1;
                    return i;
                }
                /* socket broken – try to reconnect */
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(53);
                inet_pton(AF_INET, pool_fwd_ip, &addr.sin_addr);

                int s = socket(AF_INET, SOCK_STREAM, 0);
                if (s >= 0) {
                    struct timeval tv = { DNS_TIMEOUT_SEC, 0 };
                    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
                    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                        tcp_pool[i].sock = s;
                        tcp_pool[i].in_use = 1;
                        return i;
                    } else {
                        close(s);
                    }
                }
                pthread_mutex_unlock(&tcp_pool[i].lock);
            }
        }
        usleep(1000);   /* 1 ms */
    }
}

static void return_tcp_conn(int idx) {
    tcp_pool[idx].in_use = 0;
    pthread_mutex_unlock(&tcp_pool[idx].lock);
}


/* Forward declarations */
static int resolve_internal(const char *qname, uint16_t qtype, uint16_t qclass,
                            dns_message_t *final_answer, cache_t *cache, int depth,
                            ns_record_t *ns_list, size_t ns_count);
static int build_ns_list(const dns_message_t *response, const uint8_t *raw,
                         size_t raw_len, ns_record_t **ns_list, size_t *ns_count);
static ssize_t send_query(const struct sockaddr_in *ns, const char *qname,
                          uint16_t qtype, uint16_t qclass,
                          uint8_t *recv_buf, size_t recv_buf_len);

/*  Load root hints */
int resolver_load_root_hints(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { perror("fopen root.hints"); return -1; }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Elimină newline-ul și ignoră liniile goale sau comentarii
        line[strcspn(line, "\n")] = 0;
        if (line[0] == ';' || line[0] == '\0') continue;

        // Găsește ultimul spațiu – după el se află adresa IP
        char *last_space = strrchr(line, ' ');
        if (!last_space) continue;
        char *ip_str = last_space + 1;

        // Validează IP-ul
        struct in_addr addr;
        if (inet_pton(AF_INET, ip_str, &addr) != 1) continue;

        // Extrage primul token – numele serverului
        char name[128];
        if (sscanf(line, "%127s", name) != 1) continue;

        // Adaugă în lista globală
        ns_record_t *tmp = realloc(g_root_hints, (g_root_count + 1) * sizeof(ns_record_t));
        if (!tmp) break;
        g_root_hints = tmp;

        strncpy(g_root_hints[g_root_count].name, name, DNS_MAX_NAME_LEN);
        g_root_hints[g_root_count].name[DNS_MAX_NAME_LEN] = '\0';
        g_root_hints[g_root_count].addr.sin_family = AF_INET;
        g_root_hints[g_root_count].addr.sin_addr = addr;
        g_root_hints[g_root_count].addr.sin_port = htons(DNS_PORT);
        g_root_count++;
    }
    fclose(f);
    return (g_root_count > 0) ? 0 : -1;
}

/*  Send DNS query (UDP) */
static ssize_t send_query(const struct sockaddr_in *ns, const char *qname,
                          uint16_t qtype, uint16_t qclass,
                          uint8_t *recv_buf, size_t recv_buf_len) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;
    struct timeval tv = { DNS_TIMEOUT_SEC, 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    uint8_t qbuf[EDNS_SIZE];
    dns_builder_t builder;
    dns_builder_init(&builder, qbuf, sizeof(qbuf));
    uint16_t id = (uint16_t)(rand() & 0xFFFF);
    dns_build_response_header(&builder, id, 0x0000, 1, 0, 0, 0);
    dns_build_question(&builder, qname, qtype, qclass);
    dns_build_rr(&builder, "", DNS_TYPE_OPT, EDNS_SIZE, 0, NULL, 0);
    size_t qlen = dns_builder_pos(&builder);

    if (sendto(sock, qbuf, qlen, 0, (const struct sockaddr*)ns, sizeof(*ns)) < 0) {
        close(sock); return -1;
    }
    ssize_t rlen = recvfrom(sock, recv_buf, recv_buf_len, 0, NULL, NULL);
    close(sock);
    return rlen;
}

/*  Decompress name from RR rdata */
static int decompress_name_in_rdata(const dns_rr_t *rr, const uint8_t *packet_buf,
                                    size_t packet_len, char *out, size_t out_len) {
    ptrdiff_t abs_off = rr->rdata - packet_buf;
    if (abs_off < 0 || (size_t)abs_off >= packet_len) return -1;
    size_t name_offset = abs_off;
    return read_name(packet_buf, packet_len, &name_offset, out, out_len);
}

/*  Build NS list */
static int build_ns_list(const dns_message_t *response, const uint8_t *raw,
                         size_t raw_len, ns_record_t **ns_list, size_t *ns_count) {
    *ns_list = NULL;
    *ns_count = 0;
    for (size_t i = 0; i < response->nscount; i++) {
        dns_rr_t *rr = &response->authorities[i];
        if (rr->type != DNS_TYPE_NS) continue;
        char ns_name[DNS_MAX_NAME_LEN+1];
        if (decompress_name_in_rdata(rr, raw, raw_len, ns_name, sizeof(ns_name)) < 0)
            continue;
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        int has_glue = 0;
        for (size_t j = 0; j < response->arcount; j++) {
            dns_rr_t *glue = &response->additionals[j];
            if (glue->type == DNS_TYPE_A && strcmp(glue->name, ns_name) == 0
                && glue->rdlength == 4) {
                memcpy(&addr.sin_addr, glue->rdata, 4);
                addr.sin_family = AF_INET;
                addr.sin_port = htons(DNS_PORT);
                has_glue = 1;
                break;
            }
        }
        if (!has_glue) continue;
        ns_record_t *tmp = realloc(*ns_list, (*ns_count + 1) * sizeof(ns_record_t));
        if (!tmp) break;
        *ns_list = tmp;
        strncpy((*ns_list)[*ns_count].name, ns_name, DNS_MAX_NAME_LEN);
        (*ns_list)[*ns_count].name[DNS_MAX_NAME_LEN] = '\0';
        (*ns_list)[*ns_count].addr = addr;
        (*ns_count)++;
    }
    return (*ns_count > 0) ? 0 : -1;
}

/*  Core recursive resolver */
static int resolve_internal(const char *qname, uint16_t qtype, uint16_t qclass,
                            dns_message_t *final_answer, cache_t *cache, int depth,
                            ns_record_t *ns_list, size_t ns_count) {
    if (depth > MAX_RECURSION_DEPTH) return -2;

    // Check cache
    uint8_t *cached_data = NULL;
    size_t cached_len = 0;
    if (cache_lookup(cache, qname, qtype, qclass, &cached_data, &cached_len) == 0) {
        dns_message_t tmp;
        if (dns_parse_message(cached_data, cached_len, &tmp) == 0) {
            *final_answer = tmp;
            return 0;
        }
    }

    // Check static forwarders for local test domains
    for (int i = 0; static_forwarders[i].zone != NULL; i++) {
        size_t zlen = strlen(static_forwarders[i].zone);
        size_t qlen = strlen(qname);
        const char *qcmp = qname;
        if (qlen > 0 && qname[qlen-1] == '.')
            qlen--;

        if (qlen < zlen) continue;
        if (strncasecmp(qcmp + qlen - zlen, static_forwarders[i].zone, zlen) != 0)
            continue;

        struct sockaddr_in fwd_addr;
        memset(&fwd_addr, 0, sizeof(fwd_addr));
        fwd_addr.sin_family = AF_INET;
        fwd_addr.sin_port = htons(static_forwarders[i].port);
        if (inet_pton(AF_INET, static_forwarders[i].ip, &fwd_addr.sin_addr) != 1)
            continue;
        uint8_t raw[EDNS_SIZE];
        ssize_t raw_len = send_query(&fwd_addr, qname, qtype, qclass, raw, sizeof(raw));
        if (raw_len >= 12) {
            dns_message_t resp;
            if (dns_parse_message(raw, raw_len, &resp) == 0) {
                cache_store(cache, qname, qtype, qclass, raw, raw_len,
                            resp.answers ? resp.answers[0].ttl : 300);
                *final_answer = resp;
                return 0;
            }
        }
        return -1;
    }
    // Forwarding mode – only active when DNS_FORWARDER is set
    const char *fwd_ip = getenv("DNS_FORWARDER");
    if (fwd_ip) {
        const char *tcp_flag = getenv("DNS_FORWARDER_TCP");
        int use_tcp = (tcp_flag && strcmp(tcp_flag, "0") != 0) ? 1 : 0;

        if (use_tcp) {
            /* ---------- TCP persistent forwarding ---------- */
            if (!pool_initialised) {
                //fprintf(stderr, "DEBUG: init_tcp_pool(%s)\n", fwd_ip);
                init_tcp_pool(fwd_ip);
            }

            uint8_t qbuf[EDNS_SIZE];
            dns_builder_t builder;
            dns_builder_init(&builder, qbuf, sizeof(qbuf));
            uint16_t id = (uint16_t)(rand() & 0xFFFF);
            dns_build_response_header(&builder, id, 0x0000, 1, 0, 0, 0);
            dns_build_question(&builder, qname, qtype, qclass);
            dns_build_rr(&builder, "", DNS_TYPE_OPT, EDNS_SIZE, 0, NULL, 0);
            size_t qlen = dns_builder_pos(&builder);

            uint8_t raw[EDNS_SIZE];
            ssize_t raw_len = -1;

           // fprintf(stderr, "DEBUG: borrowing TCP conn\n");
            int idx = borrow_tcp_conn();
           // fprintf(stderr, "DEBUG: got conn index %d, sock=%d\n", idx, tcp_pool[idx].sock);
            int tcp_sock = tcp_pool[idx].sock;

            uint16_t net_len = htons(qlen);
            if (send(tcp_sock, &net_len, 2, 0) == 2 &&
                send(tcp_sock, qbuf, qlen, 0) == (ssize_t)qlen &&
                recv(tcp_sock, &net_len, 2, MSG_WAITALL) == 2) {
                size_t rlen = ntohs(net_len);
                //fprintf(stderr, "DEBUG: received len=%zu\n", rlen);
                if (rlen > 0 && rlen < EDNS_SIZE) {
                    if (recv(tcp_sock, raw, rlen, MSG_WAITALL) == (ssize_t)rlen) {
                        raw_len = rlen;
                      //  fprintf(stderr, "DEBUG: received full response\n");
                        //fprintf(stderr, "DEBUG: raw response hex: ");
                      //  for (int i = 0; i < raw_len; i++) fprintf(stderr, "%02x ", raw[i]);
                        //fprintf(stderr, "\n");
                    } else {
                      //  fprintf(stderr, "DEBUG: recv body failed\n");
                    }
                }
            } else {
               // fprintf(stderr, "DEBUG: send/recv header failed\n");
                close(tcp_pool[idx].sock);
                tcp_pool[idx].sock = -1;
            }
            return_tcp_conn(idx);

            if (raw_len >= 12) {
                dns_message_t resp;
                if (dns_parse_message(raw, raw_len, &resp) == 0) {
                    cache_store(cache, qname, qtype, qclass, raw, raw_len,
                                resp.answers ? resp.answers[0].ttl : 300);
                    *final_answer = resp;
                    return 0;
                } else {
                  //  fprintf(stderr, "DEBUG: dns_parse_message failed\n");
                }
            }
            return -1;
        } else {
            /* ---------- UDP forwarding (if allowed) ---------- */
            struct sockaddr_in fwd_addr;
            memset(&fwd_addr, 0, sizeof(fwd_addr));
            fwd_addr.sin_family = AF_INET;
            fwd_addr.sin_port = htons(53);
            if (inet_pton(AF_INET, fwd_ip, &fwd_addr.sin_addr) == 1) {
                uint8_t raw[EDNS_SIZE];
                ssize_t raw_len = send_query(&fwd_addr, qname, qtype, qclass, raw, sizeof(raw));
                if (raw_len >= 12) {
                    dns_message_t resp;
                    if (dns_parse_message(raw, raw_len, &resp) == 0) {
                        cache_store(cache, qname, qtype, qclass, raw, raw_len,
                                    resp.answers ? resp.answers[0].ttl : 300);
                        *final_answer = resp;
                        return 0;
                    }
                }
            }
            return -1;
        }
    }
    // If no forwarder is set, fall through to iterative resolution below
    // Iterative resolution (only used if no forwarder is set)
    for (size_t i = 0; i < ns_count; i++) {
        ns_record_t *ns = &ns_list[i];
        uint8_t raw[EDNS_SIZE];
        ssize_t raw_len = send_query(&ns->addr, qname, qtype, qclass, raw, sizeof(raw));
        if (raw_len < 12) continue;

        dns_message_t resp;
        if (dns_parse_message(raw, raw_len, &resp) < 0) continue;

        if (resp.header.ancount > 0) {
            if (resp.answers[0].type == DNS_TYPE_CNAME) {
                char cname[DNS_MAX_NAME_LEN+1];
                if (decompress_name_in_rdata(&resp.answers[0], raw, raw_len,
                                             cname, sizeof(cname)) == 0) {
                    dns_message_t cname_answer;
                    int rc = resolve_internal(cname, qtype, qclass, &cname_answer,
                                              cache, depth+1, ns_list, ns_count);
                    if (rc < 0) {
                        dns_free_message(&resp);
                        return rc;
                    }
                    dns_free_message(&resp);
                    *final_answer = cname_answer;
                    return 0;
                }
            }
            cache_store(cache, qname, qtype, qclass, raw, raw_len, resp.answers[0].ttl);
            *final_answer = resp;
            return 0;
        }

        if (resp.header.nscount > 0) {
            ns_record_t *new_ns = NULL;
            size_t new_count = 0;
            if (build_ns_list(&resp, raw, raw_len, &new_ns, &new_count) == 0) {
                dns_message_t sub_answer;
                int rc = resolve_internal(qname, qtype, qclass, &sub_answer,
                                          cache, depth+1, new_ns, new_count);
                free(new_ns);
                dns_free_message(&resp);
                if (rc == 0) {
                    *final_answer = sub_answer;
                    return 0;
                } else if (rc == -3) {
                    return -3;
                }
            }
            dns_free_message(&resp);
            continue;
        }

        if ((resp.header.flags & 0x0F) == DNS_RCODE_NXDOMAIN) {
            cache_store(cache, qname, qtype, qclass, raw, raw_len, 600);
            *final_answer = resp;
            return -3;
        }

        dns_free_message(&resp);
    }

    return -1;
}

/*  Public entry point */
int resolver_resolve(const dns_message_t *query, dns_message_t *answer, cache_t *cache) {
    if (query->qdcount < 1) return -1;
    char qname[DNS_MAX_NAME_LEN+1];
    snprintf(qname, sizeof(qname), "%s", query->questions[0].qname);
    uint16_t qtype = query->questions[0].qtype;
    uint16_t qclass = query->questions[0].qclass;

    return resolve_internal(qname, qtype, qclass, answer, cache, 0,
                            g_root_hints, g_root_count);
}
