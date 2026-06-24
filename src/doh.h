#ifndef DOH_H
#define DOH_H

#include <stdint.h>

typedef struct doh_ctx doh_ctx_t;

doh_ctx_t* doh_init(int port, void *cache);

// Stop the DoH listener and free resources
void doh_stop(doh_ctx_t *ctx);

#endif
