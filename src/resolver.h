#ifndef RESOLVER_H
#define RESOLVER_H

#include "wire.h"
#include "cache.h"

/* (format: "A.ROOT-SERVERS.NET. 3600 A 198.41.0.4") */
int resolver_load_root_hints(const char *filename);


int resolver_resolve(const dns_message_t *query, dns_message_t *answer, cache_t *cache);

#endif
