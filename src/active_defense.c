#include "active_defense.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "reputation.h"

extern rep_ctx_t *g_rep;
#define QUARANTINE_MAX_EVENTS 5
#define QUARANTINE_WINDOW_SEC 60

typedef struct {
    uint32_t ip;
    time_t   banned_until;
} quarantine_entry_t;

typedef struct block_event {
    uint32_t client_ip;
    time_t   timestamp;
    struct block_event *next;
} block_event_t;

typedef struct ad_ctx {
    pthread_t      http_thread;
    int            http_port;
    int            http_sock;
    volatile int   running;

    quarantine_entry_t *quarantined;
    size_t             quarantined_cap;
    size_t             quarantined_cnt;

    block_event_t *events;

    pthread_mutex_t lock;
} ad_ctx_t;

static const char warning_page[] =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<html><body><h1>Access Blocked</h1>"
    "<p>Your request to a malicious domain was blocked by the DNS firewall.</p>"
    "</body></html>";
    
static void *http_thread_func(void *arg) {
    ad_ctx_t *ctx = (ad_ctx_t*)arg;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    char client_ip[INET_ADDRSTRLEN];
    char req_line[256];

    while (ctx->running) {
        int client = accept(ctx->http_sock, (struct sockaddr*)&addr, &addr_len);
        if (client < 0) {
            if (!ctx->running) break;
            continue;
        }
        inet_ntop(AF_INET, &addr.sin_addr, client_ip, sizeof(client_ip));

        // Read the request line 
        ssize_t n = recv(client, req_line, sizeof(req_line) - 1, 0);
        if (n > 0) {
            req_line[n] = '\0';
            if (strstr(req_line, "GET /rpz.zone")) {
                //Serve the RPZ zone file
                FILE *rpz = fopen("rpz.zone", "r");
                if (rpz) {
                    char rpz_buf[4096];
                    const char *hdr = "HTTP/1.0 200 OK\r\n"
                                      "Content-Type: text/plain\r\n"
                                      "Connection: close\r\n\r\n";
                    send(client, hdr, strlen(hdr), 0);
                    while (fgets(rpz_buf, sizeof(rpz_buf), rpz))
                        send(client, rpz_buf, strlen(rpz_buf), 0);
                    fclose(rpz);
                } else {
                    const char *err = "HTTP/1.0 404 Not Found\r\n"
                                      "Content-Length: 0\r\n\r\n";
                    send(client, err, strlen(err), 0);
                }
                printf("Sinkhole: served RPZ zone to %s\n", client_ip);
                close(client);
                continue;
            }
        }

        /* Default: serve the warning page */
        printf("Sinkhole: connection from %s\n", client_ip);
        write(client, warning_page, strlen(warning_page));
        close(client);
    }
    return NULL;
}

ad_ctx_t* ad_init(int http_port) {
    ad_ctx_t *ctx = calloc(1, sizeof(ad_ctx_t));
    if (!ctx) return NULL;
    ctx->http_port = http_port;
    pthread_mutex_init(&ctx->lock, NULL);

    ctx->http_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->http_sock < 0) { perror("sinkhole socket"); free(ctx); return NULL; }
    int opt = 1;
    setsockopt(ctx->http_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in saddr = {
        .sin_family = AF_INET,
        .sin_port = htons(http_port),
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(ctx->http_sock, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
        perror("sinkhole bind"); close(ctx->http_sock); free(ctx); return NULL;
    }
    listen(ctx->http_sock, 5);
    ctx->running = 1;
    if (pthread_create(&ctx->http_thread, NULL, http_thread_func, ctx) != 0) {
        ctx->running = 0; close(ctx->http_sock); free(ctx); return NULL;
    }
    return ctx;
}

void ad_destroy(ad_ctx_t *ctx) {
    if (!ctx) return;
    ctx->running = 0;
    shutdown(ctx->http_sock, SHUT_RDWR);
    close(ctx->http_sock);
    pthread_join(ctx->http_thread, NULL);
    pthread_mutex_destroy(&ctx->lock);
    block_event_t *ev = ctx->events;
    while (ev) {
        block_event_t *next = ev->next;
        free(ev);
        ev = next;
    }
    free(ctx->quarantined);
    free(ctx);
}

int ad_is_quarantined(ad_ctx_t *ctx, const struct sockaddr_in *client) {
    if (!ctx || !client) return 0;
    uint32_t ip = client->sin_addr.s_addr;
    time_t now = time(NULL);
    pthread_mutex_lock(&ctx->lock);
    for (size_t i = 0; i < ctx->quarantined_cnt; i++) {
        if (ctx->quarantined[i].ip == ip && now < ctx->quarantined[i].banned_until) {
            pthread_mutex_unlock(&ctx->lock);
            return 1;
        }
    }
    pthread_mutex_unlock(&ctx->lock);
    return 0;
}

void ad_report_block(ad_ctx_t *ctx, const struct sockaddr_in *client) {
    if (!ctx || !client) return;
    uint32_t ip = client->sin_addr.s_addr;
    time_t now = time(NULL);
    pthread_mutex_lock(&ctx->lock);

    block_event_t *ev = malloc(sizeof(block_event_t));
    ev->client_ip = ip;
    ev->timestamp = now;
    ev->next = ctx->events;
    ctx->events = ev;

    int count = 0;
    for (block_event_t *e = ctx->events; e; e = e->next) {
        if (e->client_ip == ip && now - e->timestamp < QUARANTINE_WINDOW_SEC)
            count++;
    }

    if (count >= QUARANTINE_MAX_EVENTS) {
        ctx->quarantined = realloc(ctx->quarantined,
                                   (ctx->quarantined_cnt+1) * sizeof(quarantine_entry_t));
        ctx->quarantined[ctx->quarantined_cnt].ip = ip;
        ctx->quarantined[ctx->quarantined_cnt].banned_until = now + 600;
        ctx->quarantined_cnt++;
        char ipstr[INET_ADDRSTRLEN];
        struct in_addr ia = { .s_addr = ip };
        inet_ntop(AF_INET, &ia, ipstr, sizeof(ipstr));
        printf("Quarantine: client %s banned for 10 minutes\n", ipstr);
    }

    pthread_mutex_unlock(&ctx->lock);
}

void ad_unquarantine(ad_ctx_t *ctx, const struct sockaddr_in *client) {
    if (!ctx || !client) return;
    uint32_t ip = client->sin_addr.s_addr;
    pthread_mutex_lock(&ctx->lock);
    for (size_t i = 0; i < ctx->quarantined_cnt; i++) {
        if (ctx->quarantined[i].ip == ip) {
            ctx->quarantined[i] = ctx->quarantined[ctx->quarantined_cnt-1];
            ctx->quarantined_cnt--;
            break;
        }
    }
    pthread_mutex_unlock(&ctx->lock);
}
int ad_export_rpz(ad_ctx_t *ctx, const char *filename) {
    (void)ctx;
    FILE *f = fopen(filename, "w");
    if (!f) return -1;

    fprintf(f, "$TTL 300\n"
               "@ SOA localhost. admin.localhost. (1 3600 600 86400 300)\n"
               "  NS localhost.\n\n");

    /* static blacklist */
    FILE *bl = fopen("blacklist.txt", "r");
    if (bl) {
        char line[256];
        while (fgets(line, sizeof(line), bl)) {
            line[strcspn(line, "\r\n")] = '\0';
            if (line[0] == '\0' || line[0] == '#') continue;
            fprintf(f, "%s CNAME .\n", line);
        }
        fclose(bl);
    }

    /* dynamically blocked domains from reputation tracker */
    if (g_rep) {
        char **rep_domains = NULL;
        int rep_count = 0;
        if (rep_get_blocked_domains(g_rep, &rep_domains, &rep_count) == 0) {
            for (int i = 0; i < rep_count; i++) {
                fprintf(f, "%s CNAME .\n", rep_domains[i]);
                free(rep_domains[i]);
            }
            free(rep_domains);
        }
    }

    fclose(f);
    printf("RPZ zone exported to %s\n", filename);
    return 0;
}
