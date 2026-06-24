#include "doh.h"
#include "wire.h"
#include "processor.h"      /* process_dns_query si callback‑urile asociate */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define DOH_RECV_TIMEOUT 5
#define MAX_HTTP_HEADER  8192
#define DNS_QUERY_MAX    512


/*RFC 4648  */

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int b64_decode(const char *in, size_t in_len, uint8_t *out, size_t *out_len) {
    size_t i = 0, j = 0;
    uint32_t buf = 0;
    int bits = 0;
    while (i < in_len) {
        if (in[i] == '=') break;
        const char *p = strchr(b64_table, in[i]);
        if (!p) { i++; continue; }
        buf = (buf << 6) | (p - b64_table);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[j++] = (buf >> bits) & 0xFF;
        }
        i++;
    }
    *out_len = j;
    return 0;
}

/*  Minimal HTTP request parser */

typedef enum { HTTP_GET, HTTP_POST, HTTP_UNKNOWN } http_method_t;

static int parse_http_request(const uint8_t *data, size_t len,
                              http_method_t *method, char **uri,
                              uint8_t **body, size_t *body_len) {
    const char *end_line = strstr((const char*)data, "\r\n");
    if (!end_line) return -1;
    char line[2048];
    size_t line_len = end_line - (const char*)data;
    if (line_len >= sizeof(line)) return -1;
    memcpy(line, data, line_len);
    line[line_len] = '\0';

    char method_str[16], path[1024];
    if (sscanf(line, "%s %s", method_str, path) != 2) return -1;

    if (strcasecmp(method_str, "GET") == 0) {
        *method = HTTP_GET;
        *uri = strdup(path);
        *body = NULL;
        *body_len = 0;
        return 0;
    } else if (strcasecmp(method_str, "POST") == 0) {
        *method = HTTP_POST;
        *uri = strdup(path);
        const char *body_start = strstr((const char*)data, "\r\n\r\n");
        if (!body_start) return -1;
        body_start += 4;
        *body_len = len - (body_start - (const char*)data);
        *body = malloc(*body_len);
        memcpy(*body, body_start, *body_len);
        return 0;
    }
    return -1;
}

static int extract_param(const char *uri, const char *param_name,
                         char *out, size_t out_size) {
    char search[64];
    snprintf(search, sizeof(search), "?%s=", param_name);
    const char *p = strstr(uri, search);
    if (!p) {
        snprintf(search, sizeof(search), "&%s=", param_name);
        p = strstr(uri, search);
    }
    if (!p) return -1;
    p += strlen(search);
    const char *end = p;
    while (*end && *end != '&' && *end != ' ') end++;
    size_t len = end - p;
    if (len >= out_size) return -1;
    memcpy(out, p, len);
    out[len] = '\0';
    return 0;
}

static int build_dns_query(const char *name, const char *type_str,
                           uint8_t *out, size_t *out_len) {
    uint16_t qtype = 1; /* A */
    if (strcasecmp(type_str, "AAAA") == 0) qtype = 28;
    else if (strcasecmp(type_str, "MX")   == 0) qtype = 15;
    else if (strcasecmp(type_str, "TXT")  == 0) qtype = 16;
    else if (strcasecmp(type_str, "NS")   == 0) qtype =  2;
    else if (strcasecmp(type_str, "CNAME")== 0) qtype =  5;

    uint8_t *p = out;
    *p++ = 0x00; *p++ = 0x01;   /* ID */
    *p++ = 0x01; *p++ = 0x00;   /* flags: RD */
    *p++ = 0x00; *p++ = 0x01;   /* QDCOUNT */
    *p++ = 0x00; *p++ = 0x00;   /* ANCOUNT */
    *p++ = 0x00; *p++ = 0x00;   /* NSCOUNT */
    *p++ = 0x00; *p++ = 0x00;   /* ARCOUNT */

    char tmp[256];
    strncpy(tmp, name, sizeof(tmp) - 1);
    tmp[sizeof(tmp)-1] = '\0';
    char *label = tmp;
    char *dot;
    while ((dot = strchr(label, '.')) != NULL) {
        size_t label_len = dot - label;
        if (label_len == 0) { label = dot + 1; continue; }
        *p++ = (uint8_t)label_len;
        memcpy(p, label, label_len);
        p += label_len;
        label = dot + 1;
    }
    if (*label) {
        size_t label_len = strlen(label);
        *p++ = (uint8_t)label_len;
        memcpy(p, label, label_len);
        p += label_len;
    }
    *p++ = 0x00;   /* root */
    *p++ = (qtype >> 8) & 0xFF;
    *p++ = qtype & 0xFF;
    *p++ = 0x00; *p++ = 0x01;   

    *out_len = p - out;
    return 0;
}

static int extract_dns_query_from_uri(const char *uri, uint8_t *dns_query, size_t *query_len) {
    const char *param = strstr(uri, "?dns=");
    if (!param) param = strstr(uri, "&dns=");

    if (param) {
        param += 5; /* skip "?dns=" or "&dns=" */
        const char *space = strchr(param, ' ');
        char b64[2048];
        size_t len = space ? (size_t)(space - param) : strlen(param);
        if (len >= sizeof(b64)) return -1;
        memcpy(b64, param, len);
        b64[len] = '\0';
        for (size_t i = 0; i < len; i++) {
            if (b64[i] == '-') b64[i] = '+';
            else if (b64[i] == '_') b64[i] = '/';
        }
        if (b64_decode(b64, len, dns_query, query_len) < 0) {
            fprintf(stderr, "DoH GET: base64 decode failed\n");
            return -1;
        }
        return 0;
    }

    /* fallback: name=/type= convenience format */
    char name[256] = {0};
    char type_str[16] = "A";
    if (extract_param(uri, "name", name, sizeof(name)) < 0) {
        fprintf(stderr, "DoH GET: no dns= or name= parameter in URI\n");
        return -1;
    }
    extract_param(uri, "type", type_str, sizeof(type_str));
    return build_dns_query(name, type_str, dns_query, query_len);
}

typedef struct doh_cb_arg {
    int client_fd;
} doh_cb_arg_t;

// Callback
static void doh_response_cb(void *arg, const uint8_t *data, size_t len) {
    doh_cb_arg_t *cb_arg = (doh_cb_arg_t *)arg;
    int fd = cb_arg->client_fd;

    char hdr[256];
    snprintf(hdr, sizeof(hdr),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/dns-message\r\n"
             "Content-Length: %zu\r\n"
             "\r\n", len);
    send(fd, hdr, strlen(hdr), 0);
    send(fd, data, len, 0);
    close(fd);
    free(cb_arg);
}

typedef struct {
    int client_fd;
} conn_ctx_t;

static void *handle_connection(void *arg) {
    conn_ctx_t *ctx = (conn_ctx_t*)arg;
    int fd = ctx->client_fd;
    free(ctx);

    struct timeval tv = { DOH_RECV_TIMEOUT, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    uint8_t recv_buf[65536];
    ssize_t n = recv(fd, recv_buf, sizeof(recv_buf), 0);
    if (n < 12) {
        close(fd);
        return NULL;
    }

    http_method_t method;
    char *uri = NULL;
    uint8_t *body = NULL;
    size_t body_len = 0;

    if (parse_http_request(recv_buf, n, &method, &uri, &body, &body_len) < 0) {
        const char *err = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(fd, err, strlen(err), 0);
        close(fd);
        free(uri);
        free(body);
        return NULL;
    }

    uint8_t dns_query[DNS_QUERY_MAX];
    size_t query_len = 0;
    int query_ok = 0;

    if (method == HTTP_GET) {
        if (extract_dns_query_from_uri(uri, dns_query, &query_len) == 0)
            query_ok = 1;
    } else if (method == HTTP_POST) {
        if (body_len > 0 && body_len <= DNS_QUERY_MAX) {
            memcpy(dns_query, body, body_len);
            query_len = body_len;
            query_ok = 1;
        }
    }

    free(uri);
    free(body);

    if (!query_ok) {
        const char *err = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(fd, err, strlen(err), 0);
        close(fd);
        return NULL;
    }

    /* adresa clientului pentru carantina / rate limiting */
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    if (getpeername(fd, (struct sockaddr*)&client_addr, &addr_len) != 0) {
        memset(&client_addr, 0, sizeof(client_addr));
        client_addr.sin_family = AF_INET;
        client_addr.sin_addr.s_addr = INADDR_LOOPBACK; /* fallback sigur */
    }

    doh_cb_arg_t *cb_arg = malloc(sizeof(doh_cb_arg_t));
    cb_arg->client_fd = fd;

    /* toate politicile de securitate (blacklist, DGA, fast‑flux,
       tunel, rate limit, carantina, DNSSEC, reputatie) */
    process_dns_query(dns_query, query_len,
                      &client_addr, &addr_len,
                      doh_response_cb, cb_arg);

    /* process_dns_query va trimite raspunsul prin callback,
       care inchide socketul si elibereaza cb_arg. */
    return NULL;
}


//Thread principal DoH
typedef struct doh_ctx {
    pthread_t thread;
    int listen_sock;
    volatile int running;
} doh_ctx_t;

static void *doh_listener(void *arg) {
    doh_ctx_t *ctx = (doh_ctx_t*)arg;
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    while (ctx->running) {
        int client_fd = accept(ctx->listen_sock, (struct sockaddr*)&client, &client_len);
        if (client_fd < 0) {
            if (!ctx->running) break;
            continue;
        }
        conn_ctx_t *conn = malloc(sizeof(conn_ctx_t));
        conn->client_fd = client_fd;
        pthread_t worker;
        pthread_create(&worker, NULL, handle_connection, conn);
        pthread_detach(worker);
    }
    return NULL;
}

doh_ctx_t* doh_init(int port, void *cache) {
    (void)cache;   
    doh_ctx_t *ctx = calloc(1, sizeof(doh_ctx_t));
    if (!ctx) return NULL;

    ctx->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->listen_sock < 0) { perror("doh socket"); free(ctx); return NULL; }
    int opt = 1;
    setsockopt(ctx->listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(ctx->listen_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("doh bind"); close(ctx->listen_sock); free(ctx); return NULL;
    }
    listen(ctx->listen_sock, 10);
    ctx->running = 1;
    if (pthread_create(&ctx->thread, NULL, doh_listener, ctx) != 0) {
        ctx->running = 0; close(ctx->listen_sock); free(ctx); return NULL;
    }
    printf("DoH listener started on port %d (security policies applied)\n", port);
    return ctx;
}

void doh_stop(doh_ctx_t *ctx) {
    if (!ctx) return;
    ctx->running = 0;
    shutdown(ctx->listen_sock, SHUT_RDWR);
    close(ctx->listen_sock);
    pthread_join(ctx->thread, NULL);
    free(ctx);
}
