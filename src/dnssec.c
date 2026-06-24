#include "dnssec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

struct dnssec_ctx {
    EVP_PKEY *trusted_key;
};

dnssec_ctx_t* dnssec_init(const char *trusted_key_file) {
    FILE *f = fopen(trusted_key_file, "r");
    if (!f) { perror("fopen trusted key"); return NULL; }
    EVP_PKEY *key = PEM_read_PUBKEY(f, NULL, NULL, NULL);
    fclose(f);
    if (!key) {
        fprintf(stderr, "Failed to load DNSSEC trusted key\n");
        return NULL;
    }
    dnssec_ctx_t *ctx = calloc(1, sizeof(dnssec_ctx_t));
    ctx->trusted_key = key;
    printf("DNSSEC: trusted key loaded\n");
    return ctx;
}

void dnssec_destroy(dnssec_ctx_t *ctx) {
    if (!ctx) return;
    EVP_PKEY_free(ctx->trusted_key);
    free(ctx);
}

static size_t name_to_wire(const char *name, uint8_t *out) {
    size_t pos = 0;
    while (*name) {
        const char *dot = strchr(name, '.');
        if (!dot) dot = name + strlen(name);   /* last label */
        size_t label_len = dot - name;
        if (label_len == 0) { name++; continue; }  /* skip empty labels */
        out[pos++] = (uint8_t)label_len;
        for (size_t i = 0; i < label_len; i++)
            out[pos++] = tolower((unsigned char)name[i]);
        name = dot;
        if (*name == '.') name++;
    }
    out[pos++] = 0;   /* root */
    return pos;
}


/*  Find an RRSIG covering the given owner name and type */
static const dns_rr_t* find_rrsig(const dns_message_t *msg,
                                  const char *name, uint16_t type) {
    for (size_t i = 0; i < msg->ancount; i++) {
        const dns_rr_t *rr = &msg->answers[i];
        if (rr->type == 46 && strcmp(rr->name, name) == 0) {
            if (rr->rdlength < 2) continue;
            uint16_t covered = (rr->rdata[0] << 8) | rr->rdata[1];
            if (covered == type) return rr;
        }
    }
    return NULL;
}

/*  Build the canonical signed data (RRSIG_RDATA without signature | RRset) */
static uint8_t* build_signed_data(const dns_rr_t *rrsig,
                                  const dns_rr_t **rrset, size_t rr_count,
                                  size_t *out_len) {
    const uint8_t *rd = rrsig->rdata;
    size_t rdlen = rrsig->rdlength;

    /* Locate the start of the signature field (after the signer name) */
    size_t off = 18;                      /* skip type_covered(2) algo(1) labels(1)
                                             orig_ttl(4) sig_exp(4) sig_inc(4) key_tag(2) */
    while (off < rdlen && rd[off] != 0) {
        if ((rd[off] & 0xC0) == 0xC0) { off += 2; break; }
        off += rd[off] + 1;
    }
    if (off < rdlen && rd[off] == 0) off++;
    size_t rrsig_rdata_len = off;        /* RRSIG header without signature */

    /* estimate total buffer size – add a small safety margin */
    size_t total = rrsig_rdata_len;
    for (size_t i = 0; i < rr_count; i++) {
        total += strlen(rrset[i]->name) + 2 + 10 + rrset[i]->rdlength;
    }

    uint8_t *data = malloc(total);
    if (!data) return NULL;
    size_t pos = 0;

    /*RRSIG RDATA (without signature) */
    memcpy(data + pos, rd, rrsig_rdata_len);
    pos += rrsig_rdata_len;

    /* owner name in wire format, type, class, TTL, RDATA */
    for (size_t i = 0; i < rr_count; i++) {
        const char *name = rrset[i]->name;
        uint8_t wire_name[256];
        size_t wire_len = name_to_wire(name, wire_name);
        memcpy(data + pos, wire_name, wire_len);
        pos += wire_len;

        /* TYPE (2 bytes) */
        data[pos++] = (rrset[i]->type >> 8) & 0xFF;
        data[pos++] = rrset[i]->type & 0xFF;
        /* CLASS (2 bytes) */
        data[pos++] = (rrset[i]->rrclass >> 8) & 0xFF;
        data[pos++] = rrset[i]->rrclass & 0xFF;
        /* TTL (4 bytes) */
        uint32_t ttl = rrset[i]->ttl;
        data[pos++] = (ttl >> 24) & 0xFF;
        data[pos++] = (ttl >> 16) & 0xFF;
        data[pos++] = (ttl >> 8) & 0xFF;
        data[pos++] = ttl & 0xFF;
        /* RDLENGTH (2 bytes) */
        uint16_t rdlen2 = rrset[i]->rdlength;
        data[pos++] = (rdlen2 >> 8) & 0xFF;
        data[pos++] = rdlen2 & 0xFF;
        /* RDATA */
        memcpy(data + pos, rrset[i]->rdata, rdlen2);
        pos += rdlen2;
    }

    *out_len = pos;
    return data;
}


/*  verify RRSIG against a public key */
static int verify_rrsig(const dns_rr_t *rrsig, const dns_rr_t **rrset,
                        size_t count, EVP_PKEY *key) {
    size_t signed_len = 0;
    uint8_t *signed_data = build_signed_data(rrsig, rrset, count, &signed_len);
    if (!signed_data) return -1;

    const uint8_t *rd = rrsig->rdata;
    size_t off = 18;
    while (off < rrsig->rdlength && rd[off] != 0) {
        if ((rd[off] & 0xC0) == 0xC0) { off += 2; break; }
        off += rd[off] + 1;
    }
    if (off < rrsig->rdlength && rd[off] == 0) off++;
    const uint8_t *sig = rd + off;
    size_t sig_len = rrsig->rdlength - off;

    uint8_t algo = rd[2];
    const EVP_MD *md = NULL;
    if (algo == 8) md = EVP_sha256();      /* RSA/SHA-256 */
    else if (algo == 13) md = EVP_sha256(); /* ECDSA P-256 SHA-256 */
    else {
        free(signed_data);
        return -1;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestVerifyInit(ctx, NULL, md, NULL, key);
    EVP_DigestVerifyUpdate(ctx, signed_data, signed_len);
    int ret = EVP_DigestVerifyFinal(ctx, sig, sig_len);
    EVP_MD_CTX_free(ctx);
    free(signed_data);
    return (ret == 1) ? 0 : -1;
}


/* entry point */
int dnssec_validate(dnssec_ctx_t *ctx,
                    const uint8_t *packet, size_t packet_len,
                    const dns_message_t *msg) {
    (void)packet; (void)packet_len;
    if (!ctx || msg->ancount == 0) return -2;

    for (size_t i = 0; i < msg->ancount; i++) {
        const dns_rr_t *rr = &msg->answers[i];
        if (rr->type == DNS_TYPE_A && rr->rdlength == 4) {
            const dns_rr_t *rrsig = find_rrsig(msg, rr->name, DNS_TYPE_A);
            if (!rrsig) continue;
            const dns_rr_t *rrset[1] = { rr };
            if (verify_rrsig(rrsig, rrset, 1, ctx->trusted_key) == 0)
                return 0;   /* secure */
            else
                return -1;  /* bogus */
        }
    }
    return -2;   /* no signatures found */
}
