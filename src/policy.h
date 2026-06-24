#ifndef POLICY_H
#define POLICY_H

#include "wire.h"
#include <stdint.h>
#include <time.h>

#define POLICY_ACTION_ALLOW  0
#define POLICY_ACTION_BLOCK  1
#define POLICY_ACTION_LOG    2
#define POLICY_ACTION_SINKHOLE 3

typedef struct policy_ctx policy_ctx_t;
#define DEFAULT_FLUX_IP_MAX 500
extern int g_flux_ip_max;

/* initialise policy engine: load blacklist, initialise detection structures blacklist_file can be NULL if no file. */
policy_ctx_t* policy_init(const char *blacklist_file);

/* Free resources */
void policy_destroy(policy_ctx_t *ctx);

/* check if a domain is on the static blacklist returns 1 if blocked, 0 otherwise. */
int policy_check_blocklist(policy_ctx_t *ctx, const char *domain);

/* after resolution, examine the query and response for suspicious behaviour.
   *action is set to the recommended action. returns 0 on success. */
int policy_check_post_resolve(policy_ctx_t *ctx,
                              const dns_message_t *query,
                              const dns_message_t *response,
                              const uint8_t *response_raw, size_t response_raw_len,
                              int *action);

/* Periodically clean up expired tracking data once per minute  good for memory. */
void policy_housekeeping(policy_ctx_t *ctx);

int policy_check_preresolve(const char *domain);

#endif
