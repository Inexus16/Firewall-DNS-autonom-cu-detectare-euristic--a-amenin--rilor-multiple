#ifndef DNSSEC_H
#define DNSSEC_H

#include "wire.h"

typedef struct dnssec_ctx dnssec_ctx_t;

/* format: DS or DNSKEY in wire format
   returns NULL if failure */
dnssec_ctx_t* dnssec_init(const char *trusted_key_file);

void dnssec_destroy(dnssec_ctx_t *ctx);

// validate the answer in a response 
int dnssec_validate(dnssec_ctx_t *ctx,
                    const uint8_t *packet, size_t packet_len,
                    const dns_message_t *msg);

#endif
