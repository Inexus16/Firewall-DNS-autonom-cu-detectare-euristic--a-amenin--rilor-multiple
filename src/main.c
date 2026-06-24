#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#include "wire.h"
#include "resolver.h"
#include "cache.h"
#include "policy.h"
#include "active_defense.h"
#include "doh.h"
#include "dnssec.h"
#include "rate_limit.h"
#include "reputation.h"
#include "processor.h"

#define ROOT_SERVERS_FILE "root.hints"

cache_t *g_cache = NULL;
policy_ctx_t *g_policy = NULL;
ad_ctx_t *g_ad = NULL;
doh_ctx_t *g_doh = NULL;
dnssec_ctx_t *g_dnssec = NULL;
rl_ctx_t *g_rl = NULL;
rep_ctx_t *g_rep = NULL;
static int g_sock = -1;
static int g_tcp_sock = -1;
static volatile int g_tcp_running = 1;

// Thread pool implementation
typedef struct {
    uint8_t          buf[DNS_MAX_PACKET];
    size_t           len;
    struct sockaddr_in client;
    socklen_t        client_len;
    int              sock;
} udp_job_t;

#define JOB_QUEUE_SIZE 1024
static udp_job_t job_queue[JOB_QUEUE_SIZE];
static int job_read  = 0;
static int job_write = 0;
static int job_count = 0;
static pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  job_cond  = PTHREAD_COND_INITIALIZER;
static volatile int g_running = 1;   /* graceful shutdown flag */

static void *worker_thread(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&job_mutex);
        while (job_count == 0 && g_running)
            pthread_cond_wait(&job_cond, &job_mutex);

        if (!g_running && job_count == 0) {
            pthread_mutex_unlock(&job_mutex);
            break;
        }

        /* grab one job */
        udp_job_t job = job_queue[job_read];
        job_read = (job_read + 1) % JOB_QUEUE_SIZE;
        job_count--;
        pthread_mutex_unlock(&job_mutex);

        struct udp_cb_arg udp_arg = { job.sock, job.client, job.client_len };
        process_dns_query(job.buf, job.len, &job.client, &job.client_len,
                          udp_response_cb, &udp_arg);
    }
    return NULL;
}

static void cleanup() {
    /* wake up workers so they can exit */
    g_running = 0;
    pthread_cond_broadcast(&job_cond);

    if (g_doh) doh_stop(g_doh);
    if (g_dnssec) dnssec_destroy(g_dnssec);
    if (g_cache) cache_destroy(g_cache);
    if (g_policy) policy_destroy(g_policy);
    if (g_ad) ad_destroy(g_ad);
    if (g_rl) rl_destroy(g_rl);
    if (g_rep) rep_destroy(g_rep);
    if (g_sock >= 0) close(g_sock);
    if (g_tcp_sock >= 0) {
        g_tcp_running = 0;
        shutdown(g_tcp_sock, SHUT_RDWR);
        close(g_tcp_sock);
    }
}

static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        cleanup();
        exit(0);
    }
    if (sig == SIGHUP) {
        if (g_ad) {
            ad_export_rpz(g_ad, "rpz.zone");
            if (g_rep) rep_export_csv(g_rep, "reputation.csv");
            printf("Reputation data exported to reputation.csv\n");
        }
    }
}

/* TCP listener thread (neschimbat) */
static void *tcp_listener(void *arg) {
    (void)arg;
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    while (g_tcp_running) {
        int client_fd = accept(g_tcp_sock, (struct sockaddr*)&client, &client_len);
        if (client_fd < 0) {
            if (!g_tcp_running) break;
            continue;
        }

        uint16_t len;
        if (recv(client_fd, &len, 2, MSG_WAITALL) != 2) {
            close(client_fd);
            continue;
        }
        len = ntohs(len);
        if (len > DNS_MAX_PACKET) {
            close(client_fd);
            continue;
        }

        uint8_t inbuf[DNS_MAX_PACKET];
        ssize_t n = recv(client_fd, inbuf, len, MSG_WAITALL);
        if (n != len) {
            close(client_fd);
            continue;
        }

        process_dns_query(inbuf, len, NULL, NULL, tcp_response_cb, (void *)(intptr_t)client_fd);
        close(client_fd);
    }
    return NULL;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    setbuf(stderr, NULL);   /* real-time log output */

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);

    if (resolver_load_root_hints(ROOT_SERVERS_FILE) < 0) {
        fprintf(stderr, "Failed to load root hints.\n");
        return 1;
    }

    g_cache = cache_create(CACHE_DEFAULT_SIZE);
    if (!g_cache) {
        perror("cache_create");
        return 1;
    }

    g_dnssec = dnssec_init("trusted_key.pem");
    if (!g_dnssec)
        fprintf(stderr, "Warning: DNSSEC not available (missing trusted_key.pem)\n");

    g_policy = policy_init("blacklist.txt");
    if (!g_policy) {
        fprintf(stderr, "Failed to init policy engine\n");
        return 1;
    }

    g_ad = ad_init(80);
    if (!g_ad) {
        fprintf(stderr, "Failed to init active defence\n");
        return 1;
    }

    g_doh = doh_init(8053, g_cache);
    if (!g_doh) {
        fprintf(stderr, "Failed to start DoH listener\n");
        return 1;
    }

    g_rl = rl_create(20, 256);
    if (!g_rl)
        fprintf(stderr, "Warning: rate limiter not available\n");

    g_rep = rep_init();
    if (!g_rep)
        fprintf(stderr, "Warning: reputation tracker not available\n");

    // UDP socket
    g_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sock < 0) { perror("socket"); return 1; }

    int rcvbuf = 4 * 1024 * 1024;  /* 4 MB */
    setsockopt(g_sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    // TCP socket
    g_tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_tcp_sock < 0) { perror("tcp socket"); return 1; }
    int opt = 1;
    setsockopt(g_tcp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in tcp_addr;
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(53);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(g_tcp_sock, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("tcp bind");
        return 1;
    }
    listen(g_tcp_sock, 10);
    pthread_t tcp_thread;
    pthread_create(&tcp_thread, NULL, tcp_listener, NULL);
    pthread_detach(tcp_thread);

    // Launch worker threads
    #define NUM_WORKERS 8
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_t t;
        pthread_create(&t, NULL, worker_thread, NULL);
        pthread_detach(t);
    }

    printf("DNS firewall listening on 0.0.0.0:53 (UDP and TCP) ...\n");
    printf("Sinkhole HTTP server on port 80 ...\n");

    uint8_t inbuf[DNS_MAX_PACKET];
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    time_t last_export = 0;

    while (1) {
        ssize_t n = recvfrom(g_sock, inbuf, sizeof(inbuf), 0,
                             (struct sockaddr*)&client, &client_len);
        if (n < 12) continue;

        /* enqueue job instead of creating thread */
        pthread_mutex_lock(&job_mutex);
        if (job_count < JOB_QUEUE_SIZE) {
            memcpy(job_queue[job_write].buf, inbuf, n);
            job_queue[job_write].len = n;
            job_queue[job_write].client = client;
            job_queue[job_write].client_len = client_len;
            job_queue[job_write].sock = g_sock;
            job_write = (job_write + 1) % JOB_QUEUE_SIZE;
            job_count++;
            pthread_cond_signal(&job_cond);
        }
        pthread_mutex_unlock(&job_mutex);

        /* housekeeping */
        if (g_rep) rep_decay(g_rep);
        policy_housekeeping(g_policy);

        /* automatic RPZ export every 60 seconds */
static time_t last_export = 0;
time_t now = time(NULL);
if (last_export == 0) last_export = now;   
if (now - last_export > 60) {
    if (g_ad) ad_export_rpz(g_ad, "rpz.zone");
    if (g_rep) rep_export_csv(g_rep, "reputation.csv");
    last_export = now;
}
    }

    /* never reached, but cleanup() is called by signal */
    cleanup();
    return 0;
}
