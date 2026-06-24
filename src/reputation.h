#ifndef REPUTATION_H
#define REPUTATION_H

typedef struct rep_ctx rep_ctx_t;

rep_ctx_t* rep_init(void);
void rep_destroy(rep_ctx_t *ctx);

/* Record a block event for a domain (score increment) */
void rep_record_block(rep_ctx_t *ctx, const char *domain);

/* Decay all scores (call periodically,ex. once per minute) */
void rep_decay(rep_ctx_t *ctx);

/* Export all currently tracked domains and their scores to a CSV file */
int rep_export_csv(rep_ctx_t *ctx, const char *filename);

/* Get a list of domains with score > 0 RPZ export */
int rep_get_blocked_domains(rep_ctx_t *ctx, char ***domains, int *count);

#endif
