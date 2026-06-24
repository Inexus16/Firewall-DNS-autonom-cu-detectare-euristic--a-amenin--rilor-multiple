// policy.c – complete version with blacklist, DGA (4-gram + vowel/consonant), fast-flux, tunnel
#include "policy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

/* Default thresholds – can be overridden by environment variables     */
static float g_dga_threshold = 0.48;

/* Fast-flux threshold (configurable via environment variable FLUX_IP_MAX) */
int g_flux_ip_max = DEFAULT_FLUX_IP_MAX;   // default 500

/* Utility: remove trailing dot from domain name */
static void normalise_domain(char *d) {
    size_t len = strlen(d);
    if (len > 0 && d[len-1] == '.') {
        d[len-1] = '\0';
    }
}

// STATIC BLACKLIST 
#define BLACKLIST_BUCKETS 1024

typedef struct bl_entry {
    char *domain;
    struct bl_entry *next;
} bl_entry_t;

typedef struct {
    bl_entry_t *buckets[BLACKLIST_BUCKETS];
} blacklist_t;

static unsigned int hash_domain(const char *domain) {
    unsigned int h = 5381;
    while (*domain) {
        h = ((h << 5) + h) + tolower((unsigned char)*domain);
        domain++;
    }
    return h % BLACKLIST_BUCKETS;
}

static int blacklist_contains(blacklist_t *bl, const char *domain) {
    unsigned int h = hash_domain(domain);
    bl_entry_t *e = bl->buckets[h];
    while (e) {
        if (strcasecmp(e->domain, domain) == 0) return 1;
        e = e->next;
    }
    return 0;
}

static int blacklist_add(blacklist_t *bl, const char *domain) {
    char buf[DNS_MAX_NAME_LEN+1];
    strncpy(buf, domain, sizeof(buf));
    buf[sizeof(buf)-1] = '\0';
    normalise_domain(buf);
    unsigned int h = hash_domain(buf);
    bl_entry_t *e = bl->buckets[h];
    while (e) {
        if (strcasecmp(e->domain, buf) == 0) return 0;
        e = e->next;
    }
    e = malloc(sizeof(bl_entry_t));
    if (!e) return -1;
    e->domain = strdup(buf);
    e->next = bl->buckets[h];
    bl->buckets[h] = e;
    return 0;
}

static void blacklist_free(blacklist_t *bl) {
    for (int i = 0; i < BLACKLIST_BUCKETS; i++) {
        bl_entry_t *e = bl->buckets[i];
        while (e) {
            bl_entry_t *next = e->next;
            free(e->domain);
            free(e);
            e = next;
        }
    }
}

static int blacklist_load_file(blacklist_t *bl, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0' || line[0] == '#') continue;
        blacklist_add(bl, line);
    }
    fclose(f);
    return 0;
}

//* 4-GRAM SCORING WITH HASH TABLE 
#define HASH_SIZE 65536
#define EMPTY_SLOT ((uint32_t)-1)

typedef struct {
    char   gram[4];
    float  logprob;
} ngram_entry_t;

static ngram_entry_t *hash_table = NULL;
static uint32_t hash_capacity = 0;
static float g_min_logprob = -20.0f;
static float g_log_lo = -20.0f;   // 1st percentile of benign avg logprob
static float g_log_hi = -10.34f;  // 99th percentile

static uint32_t hash_4gram(const char *gram) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < 4; i++) {
        h ^= (unsigned char)gram[i];
        h *= 16777619u;
    }
    return h;
}

void load_ngram_model(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Warning: could not load ngram model %s\n", filename);
        return;
    }
    uint32_t count;
    if (fread(&count, sizeof(count), 1, f) != 1) { fclose(f); return; }
    hash_capacity = 1;
    while (hash_capacity < count * 2) hash_capacity <<= 1;
    if (hash_capacity < HASH_SIZE) hash_capacity = HASH_SIZE;
    hash_table = calloc(hash_capacity, sizeof(ngram_entry_t));
    for (uint32_t i = 0; i < hash_capacity; i++) hash_table[i].gram[0] = '\0';
    for (uint32_t i = 0; i < count; i++) {
        char gram[4]; float logp;
        if (fread(gram, 4, 1, f) != 1) break;
        if (fread(&logp, sizeof(float), 1, f) != 1) break;
        uint32_t h = hash_4gram(gram) & (hash_capacity - 1);
        while (hash_table[h].gram[0] != '\0') h = (h + 1) & (hash_capacity - 1);
        memcpy(hash_table[h].gram, gram, 4);
        hash_table[h].logprob = logp;
    }
    fclose(f);
    fprintf(stderr, "N-gram model loaded: %d entries, hash capacity %d\n", count, hash_capacity);
}

static float ngram_score_domain(const char *domain) {
    if (!hash_table || hash_capacity == 0) return 0.0f;
    char letters[256];
    int len = 0;
    for (const char *p = domain; *p && *p != '.'; p++) {
        char c = tolower((unsigned char)*p);
        if (c >= 'a' && c <= 'z') letters[len++] = c;
    }
    if (len < 4) return 0.0f;
    float sum_logprob = 0.0f;
    int gram_count = 0;
    for (int i = 0; i <= len - 4; i++) {
        uint32_t h = hash_4gram(letters + i) & (hash_capacity - 1);
        float logp = g_min_logprob;
        while (hash_table[h].gram[0] != '\0') {
            if (memcmp(hash_table[h].gram, letters + i, 4) == 0) {
                logp = hash_table[h].logprob;
                break;
            }
            h = (h + 1) & (hash_capacity - 1);
        }
        sum_logprob += logp;
        gram_count++;
    }
    if (gram_count == 0) return 0.0f;
    float avg = sum_logprob / gram_count;
    float norm = (avg - g_log_lo) / (g_log_hi - g_log_lo);
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    return 1.0f - norm;
}

/* VOWEL/CONSONANT SCORE */
static float vowel_consonant_score(const char *domain) {
    int vowels = 0, cons = 0, alt = 0;
    int prev_type = -1;
    for (const char *p = domain; *p && *p != '.'; p++) {
        char c = tolower((unsigned char)*p);
        if (c >= 'a' && c <= 'z') {
            if (strchr("aeiou", c)) {
                if (prev_type == 1) alt++;
                prev_type = 0; vowels++;
            } else {
                if (prev_type == 0) alt++;
                prev_type = 1; cons++;
            }
        }
    }
    int total = vowels + cons;
    if (total < 2) return 0.0f;
    float ratio = (float)vowels / total;
    float ratio_score = 1.0f - fabsf(ratio - 0.45f) * 2.0f;
    if (ratio_score < 0.0f) ratio_score = 0.0f;
    float alt_score = (float)alt / (total - 1);
    return ratio_score * 0.5f + alt_score * 0.5f;
}

/// LOGISTIC REGRESSION  
static int lr_loaded = 0;
static float lr_coeffs[6];   // 5 features + intercept

void load_lr_model(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return;
    if (fread(lr_coeffs, sizeof(float), 6, f) == 6) {
        lr_loaded = 1;
        fprintf(stderr, "LR model loaded (coefficients)\n");
    }
    fclose(f);
}

static float lr_score_domain(const char *domain) {
    // Not yet implemented – will use feature vector
    (void)domain;
    return -1.0f;
}

/// DGA DETECTION combinat                                   
static float dga_score_domain(const char *domain) {
    int len = strlen(domain);
    if (len < 5) return 0.0;

    int letters = 0, digits = 0;
    int counts[26] = {0};
    int total = 0;
    int transitions = 0;
    char prev_type = 0;

    for (int i = 0; i < len; i++) {
        char c = tolower((unsigned char)domain[i]);
        if (c >= 'a' && c <= 'z') {
            counts[c - 'a']++;
            letters++;
            total++;
            if (prev_type == 2) transitions++;
            prev_type = 1;
        } else if (c >= '0' && c <= '9') {
            digits++;
            total++;
            if (prev_type == 1) transitions++;
            prev_type = 2;
        } else {
            prev_type = 0;
        }
    }
    if (total < 2) return 0.0;

    float entropy = 0.0;
    if (letters > 0) {
        for (int i = 0; i < 26; i++) {
            if (counts[i] > 0) {
                float p = (float)counts[i] / letters;
                entropy -= p * log2f(p);
            }
        }
    }

    float entropy_norm = entropy / 4.7f;
    float digit_ratio = (float)digits / len;
    float transition_score = (len > 1) ? (float)transitions / (len - 1) : 0;
    float len_score = (len > 20) ? 1.0 : (len > 12 ? 0.5 : 0.0);
    float ngram = ngram_score_domain(domain);
    float vowel_cons = vowel_consonant_score(domain);

    // If LR model is loaded and implemented, otherwise linear combination
    if (lr_loaded) {
        // placeholder
    }

    float score = entropy_norm * 0.20f
                + digit_ratio * 0.15f
                + transition_score * 0.15f
                + len_score * 0.10f
                + ngram * 0.25f
                + vowel_cons * 0.15f;
    return score;
}

//FAST‑FLUX TRACKER                                               

#define FLUX_WINDOW_SEC 600

void init_flux_threshold(void) {
    char *env_val = getenv("FLUX_IP_MAX");
    if (env_val) {
        int val = atoi(env_val);
        if (val > 0) {
            g_flux_ip_max = val;
            fprintf(stderr, "Fast-flux threshold set to %d from environment\n", g_flux_ip_max);
        }
    } else {
        fprintf(stderr, "Using default fast-flux threshold: %d\n", g_flux_ip_max);
    }
}

typedef struct flux_entry {
    char *domain;
    int num_ips;
    struct sockaddr_in *ips;
    time_t *timestamps;
    struct flux_entry *next;
} flux_entry_t;

static flux_entry_t *flux_table = NULL;

static flux_entry_t* flux_find(const char *domain) {
    flux_entry_t *e = flux_table;
    while (e) {
        if (strcmp(e->domain, domain) == 0) return e;
        e = e->next;
    }
    return NULL;
}

static void flux_add_ip(flux_entry_t *e, const struct sockaddr_in *addr) {
    time_t now = time(NULL);
    int keep = 0;
    for (int i = 0; i < e->num_ips; i++) {
        if (now - e->timestamps[i] < FLUX_WINDOW_SEC) {
            if (keep != i) {
                e->ips[keep] = e->ips[i];
                e->timestamps[keep] = e->timestamps[i];
            }
            keep++;
        }
    }
    e->num_ips = keep;
    for (int i = 0; i < e->num_ips; i++) {
        if (memcmp(&e->ips[i].sin_addr, &addr->sin_addr, sizeof(addr->sin_addr)) == 0) {
            e->timestamps[i] = now;
            return;
        }
    }
    e->num_ips++;
    e->ips = realloc(e->ips, e->num_ips * sizeof(struct sockaddr_in));
    e->timestamps = realloc(e->timestamps, e->num_ips * sizeof(time_t));
    e->ips[e->num_ips-1] = *addr;
    e->timestamps[e->num_ips-1] = now;
}

static int flux_detect(const char *domain, const struct sockaddr_in *addr) {
    flux_entry_t *e = flux_find(domain);
    if (!e) {
        e = calloc(1, sizeof(flux_entry_t));
        e->domain = strdup(domain);
        e->next = flux_table;
        flux_table = e;
    }
    flux_add_ip(e, addr);
    if (e->num_ips > g_flux_ip_max) return 1;
    return 0;
}

static void flux_cleanup(void) {
    flux_entry_t *prev = NULL, *curr = flux_table;
    time_t now = time(NULL);
    while (curr) {
        int keep = 0;
        for (int i = 0; i < curr->num_ips; i++) {
            if (now - curr->timestamps[i] < FLUX_WINDOW_SEC) {
                if (keep != i) {
                    curr->ips[keep] = curr->ips[i];
                    curr->timestamps[keep] = curr->timestamps[i];
                }
                keep++;
            }
        }
        curr->num_ips = keep;
        if (curr->num_ips == 0) {
            if (prev) prev->next = curr->next;
            else flux_table = curr->next;
            flux_entry_t *to_free = curr;
            curr = curr->next;
            free(to_free->domain);
            free(to_free->ips);
            free(to_free->timestamps);
            free(to_free);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}

//TUNNELLING DETECTION  

static int tunnelling_score(const dns_message_t *query, const dns_message_t *response) {
    if (!query || !response) return 0;
    const char *qname = query->questions[0].qname;
    int len = strlen(qname);
    if (len > 40) {
        float score = dga_score_domain(qname);
        if (score > 0.6) return 1;
    }
    for (size_t i = 0; i < response->ancount; i++) {
        if (response->answers[i].type == 16 && response->answers[i].rdlength > 0) {
            if (response->answers[i].rdlength > 200) return 1;
            int counts[256] = {0};
            for (size_t j = 0; j < response->answers[i].rdlength; j++)
                counts[response->answers[i].rdata[j]]++;
            float entropy = 0.0f;
            for (int j = 0; j < 256; j++) {
                if (counts[j] > 0) {
                    float p = (float)counts[j] / response->answers[i].rdlength;
                    entropy -= p * log2f(p);
                }
            }
            if (entropy > 6.0f) return 1;
        }
    }
    return 0;
}

// POLICY CONTEXT 

typedef struct policy_ctx {
    blacklist_t blacklist;
    time_t last_housekeeping;
} policy_ctx_t;

policy_ctx_t* policy_init(const char *blacklist_file) {
    const char *env = getenv("DGA_THRESHOLD");
    if (env) {
        g_dga_threshold = atof(env);
        fprintf(stderr, "DGA threshold set to %.2f from environment\n", g_dga_threshold);
    }
    init_flux_threshold();
    load_ngram_model("4grams.bin");        // 4‑gram model
    load_lr_model("lr_coeffs.bin");        // logistic regression 

    policy_ctx_t *ctx = calloc(1, sizeof(policy_ctx_t));
    if (blacklist_file) {
        if (blacklist_load_file(&ctx->blacklist, blacklist_file) < 0) {
            fprintf(stderr, "Warning: could not load blacklist file '%s'\n", blacklist_file);
        }
    }
    ctx->last_housekeeping = time(NULL);
    return ctx;
}

void policy_destroy(policy_ctx_t *ctx) {
    if (!ctx) return;
    blacklist_free(&ctx->blacklist);
    flux_cleanup();
    free(ctx);
    free(hash_table);   // free ngram table
}

int policy_check_blocklist(policy_ctx_t *ctx, const char *domain) {
    char buf[DNS_MAX_NAME_LEN+1];
    strncpy(buf, domain, sizeof(buf));
    buf[sizeof(buf)-1] = '\0';
    normalise_domain(buf);
    return blacklist_contains(&ctx->blacklist, buf);
}

int policy_check_post_resolve(policy_ctx_t *ctx,
                              const dns_message_t *query,
                              const dns_message_t *response,
                              const uint8_t *response_raw, size_t response_raw_len,
                              int *action) {
    (void)ctx; (void)response_raw; (void)response_raw_len;
    *action = POLICY_ACTION_ALLOW;
    float dga_score = dga_score_domain(query->questions[0].qname);
    if (dga_score > g_dga_threshold) {
        *action = POLICY_ACTION_BLOCK;
        fprintf(stderr, "ALERT: DGA detected for %s (score %.2f)\n", query->questions[0].qname, dga_score);
        return 0;
    }
    for (size_t i = 0; i < response->ancount; i++) {
        dns_rr_t *rr = &response->answers[i];
        if ((rr->type == DNS_TYPE_A || rr->type == DNS_TYPE_AAAA) && rr->rdlength > 0) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            if (rr->type == DNS_TYPE_A && rr->rdlength == 4) {
                memcpy(&addr.sin_addr, rr->rdata, 4);
            } else if (rr->type == DNS_TYPE_AAAA && rr->rdlength == 16) {
                continue;
            }
            if (flux_detect(rr->name, &addr)) {
                *action = POLICY_ACTION_BLOCK;
                fprintf(stderr, "ALERT: Fast‑flux detected for %s\n", rr->name);
                return 0;
            }
        }
    }
    if (tunnelling_score(query, response)) {
        *action = POLICY_ACTION_BLOCK;
        fprintf(stderr, "ALERT: DNS tunnelling suspected for %s\n", query->questions[0].qname);
        return 0;
    }
    return 0;
}

int policy_check_preresolve(const char *domain) {
    char buf[DNS_MAX_NAME_LEN+1];
    strncpy(buf, domain, sizeof(buf));
    buf[sizeof(buf)-1] = '\0';
    normalise_domain(buf);
    float score = dga_score_domain(buf);
    if (score > g_dga_threshold) {
        fprintf(stderr, "ALERT: DGA blocked pre-resolution for %s (score %.2f)\n", buf, score);
        return POLICY_ACTION_BLOCK;
    }
    return POLICY_ACTION_ALLOW;
}

void policy_housekeeping(policy_ctx_t *ctx) {
    time_t now = time(NULL);
    if (now - ctx->last_housekeeping > 60) {
        flux_cleanup();
        ctx->last_housekeeping = now;
    }
}
