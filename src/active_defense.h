#ifndef ACTIVE_DEFENSE_H
#define ACTIVE_DEFENSE_H

#include <stdint.h>
#include <netinet/in.h>

typedef struct ad_ctx ad_ctx_t;

ad_ctx_t* ad_init(int http_port);
void ad_destroy(ad_ctx_t *ctx);
int ad_is_quarantined(ad_ctx_t *ctx, const struct sockaddr_in *client);
void ad_report_block(ad_ctx_t *ctx, const struct sockaddr_in *client);
void ad_unquarantine(ad_ctx_t *ctx, const struct sockaddr_in *client);
int ad_export_rpz(ad_ctx_t *ctx, const char *filename);

#endif
