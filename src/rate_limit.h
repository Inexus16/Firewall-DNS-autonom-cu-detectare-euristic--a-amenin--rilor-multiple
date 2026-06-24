#ifndef RATE_LIMIT_H
#define RATE_LIMIT_H

#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>

typedef struct rl_ctx rl_ctx_t;

rl_ctx_t* rl_create(unsigned int max_queries_per_sec, unsigned int table_size);
void rl_destroy(rl_ctx_t *ctx);
int rl_check(rl_ctx_t *ctx, const struct sockaddr_in *client);

#endif
