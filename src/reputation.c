#include "reputation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define REP_HASH_SIZE 256
#define DECAY_INTERVAL 60   /* secunde */

typedef struct rep_entry {
    char   *domain;
    int     score;
    time_t  last_update;
    struct rep_entry *next;
} rep_entry_t;

typedef struct rep_ctx {
    rep_entry_t *buckets[REP_HASH_SIZE];
    time_t       last_decay;
} rep_ctx_t;

static unsigned int hash_domain(const char *d) {
    unsigned int h = 5381;
    while (*d) { h = ((h << 5) + h) + (unsigned char)*d; d++; }
    return h % REP_HASH_SIZE;
}

rep_ctx_t* rep_init(void) {
    rep_ctx_t *ctx = calloc(1, sizeof(rep_ctx_t));
    ctx->last_decay = time(NULL);
    return ctx;
}

void rep_destroy(rep_ctx_t *ctx) {
    if (!ctx) return;
    for (int i = 0; i < REP_HASH_SIZE; i++) {
        rep_entry_t *e = ctx->buckets[i];
        while (e) {
            rep_entry_t *next = e->next;
            free(e->domain);
            free(e);
            e = next;
        }
    }
    free(ctx);
}

void rep_record_block(rep_ctx_t *ctx, const char *domain) {
    if (!ctx || !domain) return;
    unsigned int h = hash_domain(domain);
    rep_entry_t *e = ctx->buckets[h];
    while (e) {
        if (strcmp(e->domain, domain) == 0) {
            e->score++;
            e->last_update = time(NULL);
            return;
        }
        e = e->next;
    }
    /* new entry */
    e = malloc(sizeof(rep_entry_t));
    if (!e) return;
    e->domain = strdup(domain);
    e->score = 1;
    e->last_update = time(NULL);
    e->next = ctx->buckets[h];
    ctx->buckets[h] = e;
}

void rep_decay(rep_ctx_t *ctx) {
    if (!ctx) return;
    time_t now = time(NULL);
    if (now - ctx->last_decay < DECAY_INTERVAL) return;
    ctx->last_decay = now;

    for (int i = 0; i < REP_HASH_SIZE; i++) {
        rep_entry_t *prev = NULL, *e = ctx->buckets[i];
        while (e) {
            if (now - e->last_update >= DECAY_INTERVAL) {
                e->score--;
                if (e->score <= 0) {
                    /* remove entry */
                    if (prev) prev->next = e->next;
                    else ctx->buckets[i] = e->next;
                    rep_entry_t *tmp = e;
                    e = e->next;
                    free(tmp->domain);
                    free(tmp);
                    continue;
                }
                e->last_update = now;
            }
            prev = e;
            e = e->next;
        }
    }
}

int rep_export_csv(rep_ctx_t *ctx, const char *filename) {
    if (!ctx) return -1;
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    for (int i = 0; i < REP_HASH_SIZE; i++) {
        rep_entry_t *e = ctx->buckets[i];
        while (e) {
            fprintf(f, "%s\t%d\n", e->domain, e->score);
            e = e->next;
        }
    }
    fclose(f);
    return 0;
}

int rep_get_blocked_domains(rep_ctx_t *ctx, char ***domains, int *count) {
    if (!ctx) return -1;
    *count = 0;
    *domains = NULL;
    for (int i = 0; i < REP_HASH_SIZE; i++) {
        rep_entry_t *e = ctx->buckets[i];
        while (e) {
            if (e->score > 0) {
                (*domains) = realloc(*domains, (*count + 1) * sizeof(char*));
                (*domains)[*count] = strdup(e->domain);
                (*count)++;
            }
            e = e->next;
        }
    }
    return 0;
}
