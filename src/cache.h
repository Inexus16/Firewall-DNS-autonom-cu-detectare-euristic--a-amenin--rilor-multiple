#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <pthread.h>

#define CACHE_DEFAULT_SIZE 1024

typedef struct {
    char    *key;
    uint8_t *data;
    size_t   datalen;
    time_t   expiry;
    int      used;
} cache_entry_t;

typedef struct {
    cache_entry_t *entries;
    size_t         capacity;
    size_t         count;
    pthread_mutex_t lock;          /* protects all fields */
} cache_t;

cache_t* cache_create(size_t capacity);
void cache_destroy(cache_t *cache);

int  cache_lookup(cache_t *cache, const char *name, uint16_t type, uint16_t rrclass,
                  uint8_t **data, size_t *datalen);
void cache_store(cache_t *cache, const char *name, uint16_t type, uint16_t rrclass,
                 const uint8_t *data, size_t datalen, uint32_t ttl);
void cache_clean_expired(cache_t *cache);

#endif
