#include "wire.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

/* Helper: read big-endian uint16 */
static inline uint16_t read_u16(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

/* Helper: read big-endian uint32 */
static inline uint32_t read_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/* Write big-endian uint16 into buffer at current pos */
static inline void write_u16(uint8_t *p, uint16_t v) {
    p[0] = (v >> 8) & 0xFF;
    p[1] = v & 0xFF;
}

/* Write big-endian uint32 */
static inline void write_u32(uint8_t *p, uint32_t v) {
    p[0] = (v >> 24) & 0xFF;
    p[1] = (v >> 16) & 0xFF;
    p[2] = (v >> 8) & 0xFF;
    p[3] = v & 0xFF;
}

/*  Name decompression (reads from whole packet, handles compression pointers) */
int read_name(const uint8_t *pkt, size_t pkt_len, size_t *offset,
                     char *out, size_t out_len) {
    size_t pos = *offset;
    size_t out_pos = 0;
    int jumped = 0;
    size_t jump_offset = 0;
    int loops = 0;  // no compression loops

    while (1) {
        if (loops++ > 20) return -1;   // max 20 hops to prevent infinite loops
        if (pos >= pkt_len) return -1;
        uint8_t len = pkt[pos];

        if (len == 0) {
            pos++;
            break;
        }
        if ((len & 0xC0) == 0xC0) {   // compression pointer
            if (pos + 1 >= pkt_len) return -1;
            uint16_t ptr = ((len & 0x3F) << 8) | pkt[pos+1];
            if (ptr >= pkt_len) return -1;
            if (!jumped) {
                jump_offset = pos + 2; 
            }
            pos = ptr;
            jumped = 1;
            continue;
        }
        // normal label
        if (len > DNS_MAX_LABEL_LEN) return -1;
        pos++;
        if (pos + len > pkt_len) return -1;
        if (out_pos + len + 1 >= out_len) return -1; // +1 for .
        memcpy(out + out_pos, pkt + pos, len);
        out_pos += len;
        out[out_pos++] = '.';
        pos += len;
    }

    if (out_pos == 0) {
        out[0] = '.';
        out[1] = '\0';
    } else {
        out[out_pos] = '\0';  
    }
    *offset = jumped ? jump_offset : pos;
    return 0;
}  

/* Read a single resource record */
static int read_rr(const uint8_t *pkt, size_t pkt_len, size_t *offset, dns_rr_t *rr) {
    if (read_name(pkt, pkt_len, offset, rr->name, DNS_MAX_NAME_LEN) < 0)
        return -1;
    if (*offset + 10 > pkt_len) return -1;
    rr->type     = read_u16(pkt + *offset);
    rr->rrclass  = read_u16(pkt + *offset + 2);
    rr->ttl      = read_u32(pkt + *offset + 4);
    rr->rdlength = read_u16(pkt + *offset + 8);
    *offset += 10;
    if (*offset + rr->rdlength > pkt_len) return -1;
    rr->rdata = (uint8_t *)(pkt + *offset);   // points into buffer
    *offset += rr->rdlength;
    return 0;
}

/* Full DNS message */
int dns_parse_message(const uint8_t *buf, size_t buflen, dns_message_t *msg) {
    if (buflen < 12) return -1;
    memset(msg, 0, sizeof(*msg));

    msg->header.id      = read_u16(buf);
    msg->header.flags   = read_u16(buf+2);
    msg->header.qdcount = read_u16(buf+4);
    msg->header.ancount = read_u16(buf+6);
    msg->header.nscount = read_u16(buf+8);
    msg->header.arcount = read_u16(buf+10);

    size_t offset = 12;

    /* Questions */
    msg->qdcount = msg->header.qdcount;
    if (msg->qdcount > 0) {
        msg->questions = calloc(msg->qdcount, sizeof(dns_question_t));
        if (!msg->questions) return -1;
        for (uint16_t i = 0; i < msg->qdcount; i++) {
            if (read_name(buf, buflen, &offset, msg->questions[i].qname, DNS_MAX_NAME_LEN) < 0)
                return -1;
            if (offset + 4 > buflen) return -1;
            msg->questions[i].qtype  = read_u16(buf + offset);
            msg->questions[i].qclass = read_u16(buf + offset + 2);
            offset += 4;
        }
    }

    /* Answers */
    msg->ancount = msg->header.ancount;
    if (msg->ancount > 0) {
        msg->answers = calloc(msg->ancount, sizeof(dns_rr_t));
        if (!msg->answers) return -1;
        for (uint16_t i = 0; i < msg->ancount; i++) {
            if (read_rr(buf, buflen, &offset, &msg->answers[i]) < 0)
                return -1;
        }
    }

    /* Authorities */
    msg->nscount = msg->header.nscount;
    if (msg->nscount > 0) {
        msg->authorities = calloc(msg->nscount, sizeof(dns_rr_t));
        if (!msg->authorities) return -1;
        for (uint16_t i = 0; i < msg->nscount; i++) {
            if (read_rr(buf, buflen, &offset, &msg->authorities[i]) < 0)
                return -1;
        }
    }

    /* Additionals */
    msg->arcount = msg->header.arcount;
    if (msg->arcount > 0) {
        msg->additionals = calloc(msg->arcount, sizeof(dns_rr_t));
        if (!msg->additionals) return -1;
        for (uint16_t i = 0; i < msg->arcount; i++) {
            if (read_rr(buf, buflen, &offset, &msg->additionals[i]) < 0)
                return -1;
        }
    }

    return 0;
}

/* Message */
void dns_free_message(dns_message_t *msg) {
    free(msg->questions);
    free(msg->answers);
    free(msg->authorities);
    free(msg->additionals);
    memset(msg, 0, sizeof(*msg));
}

/* Debug print */
void dns_print_message(const dns_message_t *msg) {
    printf("ID: %u, Flags: 0x%04x, QD: %u, AN: %u, NS: %u, AR: %u\n",
           msg->header.id, msg->header.flags,
           msg->header.qdcount, msg->header.ancount,
           msg->header.nscount, msg->header.arcount);
    for (uint16_t i = 0; i < msg->qdcount; i++) {
        printf("  Q %u: %s (type %u, class %u)\n",
               i, msg->questions[i].qname,
               msg->questions[i].qtype, msg->questions[i].qclass);
    }
    for (uint16_t i = 0; i < msg->ancount; i++) {
        printf("  AN %u: %s type %u TTL %u\n", i, msg->answers[i].name,
               msg->answers[i].type, msg->answers[i].ttl);
    }
    for (uint16_t i = 0; i < msg->nscount; i++) {
        printf("  NS %u: %s type %u\n", i, msg->authorities[i].name,
               msg->authorities[i].type);
    }
    for (uint16_t i = 0; i < msg->arcount; i++) {
        printf("  AR %u: %s type %u\n", i, msg->additionals[i].name,
               msg->additionals[i].type);
    }
}

/* Builder API – serialize DNS responses */
void dns_builder_init(dns_builder_t *b, uint8_t *buffer, size_t buflen) {
    b->buf = buffer;
    b->buflen = buflen;
    b->pos = 0;
}

size_t dns_builder_pos(const dns_builder_t *b) {
    return b->pos;
}

static int builder_ensure(dns_builder_t *b, size_t bytes) {
    if (b->pos + bytes > b->buflen) return -1;
    return 0;
}

/* Write a domain name uncompressed */
static int write_name_uncompressed(dns_builder_t *b, const char *name) {
    if (name[0] == '\0') return -1;
    const char *start = name;
    while (*start) {
        const char *dot = strchr(start, '.');
        size_t len = dot ? (size_t)(dot - start) : strlen(start);
        if (len > DNS_MAX_LABEL_LEN) return -1;
        if (builder_ensure(b, 1 + len) < 0) return -1;
        b->buf[b->pos++] = (uint8_t)len;
        memcpy(b->buf + b->pos, start, len);
        b->pos += len;
        start += len;
        if (*start == '.') start++;
        if (*start == '\0') break;
    }
    // root label
    if (builder_ensure(b, 1) < 0) return -1;
    b->buf[b->pos++] = 0;
    return 0;
}

int dns_build_response_header(dns_builder_t *b, uint16_t id, uint16_t flags,
                              uint16_t qdcount, uint16_t ancount, uint16_t nscount, uint16_t arcount) {
    if (builder_ensure(b, 12) < 0) return -1;
    write_u16(b->buf + b->pos, id);      b->pos += 2;
    write_u16(b->buf + b->pos, flags);   b->pos += 2;
    write_u16(b->buf + b->pos, qdcount); b->pos += 2;
    write_u16(b->buf + b->pos, ancount); b->pos += 2;
    write_u16(b->buf + b->pos, nscount); b->pos += 2;
    write_u16(b->buf + b->pos, arcount); b->pos += 2;
    return 0;
}

int dns_build_question(dns_builder_t *b, const char *qname, uint16_t qtype, uint16_t qclass) {
    if (write_name_uncompressed(b, qname) < 0) return -1;
    if (builder_ensure(b, 4) < 0) return -1;
    write_u16(b->buf + b->pos, qtype);  b->pos += 2;
    write_u16(b->buf + b->pos, qclass); b->pos += 2;
    return 0;
}

int dns_build_rr(dns_builder_t *b, const char *name, uint16_t type, uint16_t rrclass,
                 uint32_t ttl, const uint8_t *rdata, uint16_t rdlength) {
    if (write_name_uncompressed(b, name) < 0) return -1;
    if (builder_ensure(b, 10 + rdlength) < 0) return -1;
    write_u16(b->buf + b->pos, type);     b->pos += 2;
    write_u16(b->buf + b->pos, rrclass);  b->pos += 2;
    write_u32(b->buf + b->pos, ttl);      b->pos += 4;
    write_u16(b->buf + b->pos, rdlength); b->pos += 2;
    if (rdlength > 0) {
        memcpy(b->buf + b->pos, rdata, rdlength);
        b->pos += rdlength;
    }
    return 0;
}

int dns_build_rr_from_rr(dns_builder_t *b, const dns_rr_t *rr) {
    return dns_build_rr(b, rr->name, rr->type, rr->rrclass, rr->ttl, rr->rdata, rr->rdlength);
}
