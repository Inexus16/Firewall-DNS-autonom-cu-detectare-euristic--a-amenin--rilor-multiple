#ifndef WIRE_H
#define WIRE_H

#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DNS_TC         0x0200   

#define DNS_MAX_NAME_LEN  255
#define DNS_MAX_LABEL_LEN 63
#define DNS_PORT          53
#define DNS_MAX_PACKET    512     // DNS UDP 

/* DNS Header (12 bytes) */
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header_t;

/* Flags */
#define DNS_QR_RESPONSE  0x8000
#define DNS_OPCODE_QUERY 0
#define DNS_RCODE_NOERROR  0
#define DNS_RCODE_FORMERR  1
#define DNS_RCODE_SERVFAIL 2
#define DNS_RCODE_NXDOMAIN 3
#define DNS_RCODE_REFUSED  5

/* RR types we care about */
#define DNS_TYPE_A     1
#define DNS_TYPE_NS    2
#define DNS_TYPE_CNAME 5
#define DNS_TYPE_SOA   6
#define DNS_TYPE_PTR   12
#define DNS_TYPE_AAAA  28
#define DNS_TYPE_OPT   41   // EDNS
#define DNS_CLASS_IN   1

/* Question*/
typedef struct {
    char     qname[DNS_MAX_NAME_LEN + 1];  // +1 for the .
    uint16_t qtype;
    uint16_t qclass;
} dns_question_t;

/* Resource Record (general) */
typedef struct {
    char     name[DNS_MAX_NAME_LEN + 1];
    uint16_t type;
    uint16_t rrclass;
    uint32_t ttl;
    uint16_t rdlength;
    uint8_t *rdata;    // points to buffer 
} dns_rr_t;

/* DNS message */
typedef struct {
    dns_header_t   header;
    dns_question_t *questions;
    dns_rr_t       *answers;
    dns_rr_t       *authorities;
    dns_rr_t       *additionals;
    size_t         qdcount, ancount, nscount, arcount;  // actual count 
} dns_message_t;

/* API */
int  dns_parse_message(const uint8_t *buf, size_t buflen, dns_message_t *msg);
void dns_free_message(dns_message_t *msg);
void dns_print_message(const dns_message_t *msg);

/* biilder API */
typedef struct {
    uint8_t *buf;
    size_t   buflen;
    size_t   pos;
} dns_builder_t;

void dns_builder_init(dns_builder_t *b, uint8_t *buffer, size_t buflen);
int  dns_build_response_header(dns_builder_t *b, uint16_t id, uint16_t flags, uint16_t qdcount,
                               uint16_t ancount, uint16_t nscount, uint16_t arcount);
int  dns_build_question(dns_builder_t *b, const char *qname, uint16_t qtype, uint16_t qclass);
int  dns_build_rr(dns_builder_t *b, const char *name, uint16_t type, uint16_t rrclass,
                  uint32_t ttl, const uint8_t *rdata, uint16_t rdlength);
int  dns_build_rr_from_rr(dns_builder_t *b, const dns_rr_t *rr);  // copy a parsed RR
size_t dns_builder_pos(const dns_builder_t *b);

/* name decompression (used by resolver) */
int read_name(const uint8_t *pkt, size_t pkt_len, size_t *offset,
              char *out, size_t out_len);

#endif
