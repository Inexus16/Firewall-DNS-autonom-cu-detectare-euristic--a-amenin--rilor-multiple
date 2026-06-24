#include "rate_limit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

typedef struct bucket {
    uint32_t ip;
    time_t   last_refill;
    unsigned int tokens;
    struct bucket *next;
} bucket_t;

typedef struct rl_ctx {
    unsigned int max_tokens;
    unsigned int table_size;
    bucket_t **buckets;
    pthread_mutex_t lock;        /* protects buckets */
} rl_ctx_t;

rl_ctx_t* rl_create(unsigned int max_queries_per_sec, unsigned int table_size) {
    rl_ctx_t *ctx = calloc(1, sizeof(rl_ctx_t));
    if (!ctx) return NULL;
    ctx->max_tokens = max_queries_per_sec;
    ctx->table_size = table_size;
    ctx->buckets = calloc(table_size, sizeof(bucket_t *));
    if (!ctx->buckets) {
        free(ctx);
        return NULL;
    }
    pthread_mutex_init(&ctx->lock, NULL);
    return ctx;
}

void rl_destroy(rl_ctx_t *ctx) {
    if (!ctx) return;
    pthread_mutex_lock(&ctx->lock);
    for (unsigned int i = 0; i < ctx->table_size; i++) {
        bucket_t *b = ctx->buckets[i];
        while (b) {
            bucket_t *next = b->next;
            free(b);
            b = next;
        }
    }
    free(ctx->buckets);
    pthread_mutex_unlock(&ctx->lock);
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
}

int rl_check(rl_ctx_t *ctx, const struct sockaddr_in *client) {
    if (!ctx || !client) return 1;
    pthread_mutex_lock(&ctx->lock);
    uint32_t ip = client->sin_addr.s_addr;
    unsigned int idx = ip % ctx->table_size;
    time_t now = time(NULL);

    bucket_t *b = ctx->buckets[idx];
    while (b) {
        if (b->ip == ip) break;
        b = b->next;
    }
    
    if (!b) {
        b = malloc(sizeof(bucket_t));
        if (!b) { pthread_mutex_unlock(&ctx->lock); return 1; }
        b->ip = ip;
        b->last_refill = now;
        b->tokens = ctx->max_tokens - 1;
        b->next = ctx->buckets[idx];
        ctx->buckets[idx] = b;
        pthread_mutex_unlock(&ctx->lock);
        return 1;
    }

    unsigned int elapsed = (unsigned int)(now - b->last_refill);
    if (elapsed > 0) {
        b->tokens += elapsed;
        if (b->tokens > ctx->max_tokens)
            b->tokens = ctx->max_tokens;
        b->last_refill = now;
    }

    if (b->tokens > 0) {
        b->tokens--;
        pthread_mutex_unlock(&ctx->lock);
        return 1;
    }

    // Rate limit hit: drop packet
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->sin_addr), ipstr, sizeof(ipstr));
    printf("Rate limit: dropping packet from %s\n", ipstr);
    pthread_mutex_unlock(&ctx->lock);
    return 0;
}
