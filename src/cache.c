#include "cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static uint32_t hash_key(const char *key) {
    uint32_t hash = 2166136261u;
    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= 16777619;
    }
    return hash;
}

cache_t* cache_create(size_t capacity) {
    cache_t *c = calloc(1, sizeof(cache_t));
    if (!c) return NULL;
    c->capacity = capacity;
    c->entries = calloc(capacity, sizeof(cache_entry_t));
    if (!c->entries) { free(c); return NULL; }
    pthread_mutex_init(&c->lock, NULL);
    return c;
}

void cache_destroy(cache_t *c) {
    if (!c) return;
    pthread_mutex_lock(&c->lock);
    for (size_t i = 0; i < c->capacity; i++) {
        cache_entry_t *e = &c->entries[i];
        if (e->used) {
            free(e->key);
            free(e->data);
        }
    }
    free(c->entries);
    pthread_mutex_unlock(&c->lock);
    pthread_mutex_destroy(&c->lock);
    free(c);
}

static void build_key(const char *name, uint16_t type, uint16_t rrclass, char *buf, size_t buflen) {
    snprintf(buf, buflen, "%s:%u:%u", name, type, rrclass);
}

int cache_lookup(cache_t *c, const char *name, uint16_t type, uint16_t rrclass,
                 uint8_t **data, size_t *datalen) {
    char key[512];
    build_key(name, type, rrclass, key, sizeof(key));
    uint32_t h = hash_key(key) % c->capacity;

    pthread_mutex_lock(&c->lock);
    for (size_t i = 0; i < c->capacity; i++) {
        size_t idx = (h + i) % c->capacity;
        cache_entry_t *e = &c->entries[idx];
        if (!e->used) break;
        if (e->used && strcmp(e->key, key) == 0) {
            if (time(NULL) > e->expiry) {
                /* expired – remove it */
                free(e->key);
                free(e->data);
                e->used = 0;
                c->count--;
                pthread_mutex_unlock(&c->lock);
                return -1;
            }
            *data = e->data;
            *datalen = e->datalen;
            pthread_mutex_unlock(&c->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&c->lock);
    return -1;
}

void cache_store(cache_t *c, const char *name, uint16_t type, uint16_t rrclass,
                 const uint8_t *data, size_t datalen, uint32_t ttl) {
    if (ttl == 0) return;

    char key[512];
    build_key(name, type, rrclass, key, sizeof(key));
    uint32_t h = hash_key(key) % c->capacity;

    pthread_mutex_lock(&c->lock);
    for (size_t i = 0; i < c->capacity; i++) {
        size_t idx = (h + i) % c->capacity;
        cache_entry_t *e = &c->entries[idx];
        if (!e->used) {
            e->key = strdup(key);
            e->data = malloc(datalen);
            memcpy(e->data, data, datalen);
            e->datalen = datalen;
            e->expiry = time(NULL) + ttl;
            e->used = 1;
            c->count++;
            pthread_mutex_unlock(&c->lock);
            return;
        }
        if (e->used && strcmp(e->key, key) == 0) {
            free(e->data);
            e->data = malloc(datalen);
            memcpy(e->data, data, datalen);
            e->datalen = datalen;
            e->expiry = time(NULL) + ttl;
            pthread_mutex_unlock(&c->lock);
            return;
        }
    }
    pthread_mutex_unlock(&c->lock);
}

void cache_clean_expired(cache_t *c) {
    time_t now = time(NULL);
    pthread_mutex_lock(&c->lock);
    for (size_t i = 0; i < c->capacity; i++) {
        cache_entry_t *e = &c->entries[i];
        if (e->used && now > e->expiry) {
            free(e->key);
            free(e->data);
            e->used = 0;
            c->count--;
        }
    }
    pthread_mutex_unlock(&c->lock);
}
