#include "processor.h"
#include "wire.h"
#include "resolver.h"
#include "cache.h"
#include "policy.h"
#include "active_defense.h"
#include "dnssec.h"
#include "rate_limit.h"
#include "reputation.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

extern cache_t *g_cache;
extern policy_ctx_t *g_policy;
extern ad_ctx_t *g_ad;
extern dnssec_ctx_t *g_dnssec;
extern rl_ctx_t *g_rl;
extern rep_ctx_t *g_rep;

void udp_response_cb(void *arg, const uint8_t *data, size_t len) {
    struct udp_cb_arg *udp = (struct udp_cb_arg *)arg;
    sendto(udp->sock, data, len, 0, (struct sockaddr *)&udp->client, udp->client_len);
}

void tcp_response_cb(void *arg, const uint8_t *data, size_t len) {
    int fd = (intptr_t)arg;
    uint16_t net_len = htons(len);
    send(fd, &net_len, 2, 0);
    send(fd, data, len, 0);
}


void process_dns_query(
    uint8_t *inbuf, size_t inbuf_len,
    struct sockaddr_in *client, socklen_t *client_len,
    response_callback_t response_cb, void *cb_arg)
{
    (void)client_len;   /* sterge */

    dns_message_t query;
    if (dns_parse_message(inbuf, inbuf_len, &query) < 0) {
        uint8_t err_buf[12];
        dns_builder_t err;
        dns_builder_init(&err, err_buf, sizeof(err_buf));
        dns_build_response_header(&err, 0, htons(DNS_QR_RESPONSE | DNS_RCODE_SERVFAIL),
                                   0, 0, 0, 0);
        response_cb(cb_arg, err_buf, dns_builder_pos(&err));
        dns_free_message(&query);
        return;
    }

    int is_localhost = (client && client->sin_addr.s_addr == htonl(INADDR_LOOPBACK));

    // Rate limit check 
    if (client && !is_localhost && g_rl && !rl_check(g_rl, client)) {
        dns_free_message(&query);
        return;
    }

    // Quarantine check 
    if (client && !is_localhost && ad_is_quarantined(g_ad, client)) {
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client->sin_addr, ipstr, sizeof(ipstr));
        printf("Quarantined client blocked: %s\n", ipstr);
        uint8_t buf[512];
        dns_builder_t b;
        dns_builder_init(&b, buf, sizeof(buf));
        dns_build_response_header(&b, query.header.id,
                                  htons(DNS_QR_RESPONSE | DNS_RCODE_NOERROR),
                                  query.qdcount, 1, 0, 0);
        for (size_t i = 0; i < query.qdcount; i++)
            dns_build_question(&b, query.questions[i].qname,
                               query.questions[i].qtype, query.questions[i].qclass);
        uint8_t sinkhole_ip[4] = {0,0,0,0};
        dns_build_rr(&b, query.questions[0].qname, DNS_TYPE_A, DNS_CLASS_IN, 300,
                     sinkhole_ip, 4);
        response_cb(cb_arg, buf, dns_builder_pos(&b));
        dns_free_message(&query);
        return;
    }

    // Pre‑resolution policy check blacklist
    if (policy_check_blocklist(g_policy, query.questions[0].qname)) {
        printf("Blocked (blacklist): %s\n", query.questions[0].qname);
        uint8_t buf[512];
        dns_builder_t b;
        dns_builder_init(&b, buf, sizeof(buf));
        dns_build_response_header(&b, query.header.id,
                                  htons(DNS_QR_RESPONSE | DNS_RCODE_NOERROR),
                                  query.qdcount, 1, 0, 0);
        for (size_t i = 0; i < query.qdcount; i++)
            dns_build_question(&b, query.questions[i].qname,
                               query.questions[i].qtype, query.questions[i].qclass);
        uint8_t sinkhole_ip[4] = {0,0,0,0};
        dns_build_rr(&b, query.questions[0].qname, DNS_TYPE_A, DNS_CLASS_IN, 300,
                     sinkhole_ip, 4);
        response_cb(cb_arg, buf, dns_builder_pos(&b));
        if (client && !is_localhost)
            ad_report_block(g_ad, client);
        if (g_rep) rep_record_block(g_rep, query.questions[0].qname);
        dns_free_message(&query);
        return;
    }

    // Pre‑resolution DGA check 
    if (policy_check_preresolve(query.questions[0].qname) == POLICY_ACTION_BLOCK) {
        printf("Blocked (DGA pre‑resolve): %s\n", query.questions[0].qname);
        uint8_t buf[512];
        dns_builder_t b;
        dns_builder_init(&b, buf, sizeof(buf));
        dns_build_response_header(&b, query.header.id,
                                  htons(DNS_QR_RESPONSE | DNS_RCODE_NOERROR),
                                  query.qdcount, 1, 0, 0);
        for (size_t i = 0; i < query.qdcount; i++)
            dns_build_question(&b, query.questions[i].qname,
                               query.questions[i].qtype, query.questions[i].qclass);
        uint8_t sinkhole_ip[4] = {0,0,0,0};
        dns_build_rr(&b, query.questions[0].qname, DNS_TYPE_A, DNS_CLASS_IN, 300,
                     sinkhole_ip, 4);
        response_cb(cb_arg, buf, dns_builder_pos(&b));
        if (client && !is_localhost)
            ad_report_block(g_ad, client);
        if (g_rep) rep_record_block(g_rep, query.questions[0].qname);
        dns_free_message(&query);
        return;
    }

    // Recursive resolution 
    dns_message_t answer;
    memset(&answer, 0, sizeof(answer));
    int rc = resolver_resolve(&query, &answer, g_cache);

    // Post‑resolution policy check 
    int action = POLICY_ACTION_ALLOW;
    if (rc == 0 && answer.ancount > 0) {
        policy_check_post_resolve(g_policy, &query, &answer, NULL, 0, &action);
    }

    if (action == POLICY_ACTION_BLOCK) {
        printf("Blocked (dynamic rule): %s\n", query.questions[0].qname);
        uint8_t buf[512];
        dns_builder_t b;
        dns_builder_init(&b, buf, sizeof(buf));
        dns_build_response_header(&b, query.header.id,
                                  htons(DNS_QR_RESPONSE | DNS_RCODE_NOERROR),
                                  query.qdcount, 1, 0, 0);
        for (size_t i = 0; i < query.qdcount; i++)
            dns_build_question(&b, query.questions[i].qname,
                               query.questions[i].qtype, query.questions[i].qclass);
        uint8_t sinkhole_ip[4] = {0,0,0,0};
        dns_build_rr(&b, query.questions[0].qname, DNS_TYPE_A, DNS_CLASS_IN, 300,
                     sinkhole_ip, 4);
        response_cb(cb_arg, buf, dns_builder_pos(&b));
        if (client && !is_localhost)
            ad_report_block(g_ad, client);
        if (g_rep) rep_record_block(g_rep, query.questions[0].qname);
        dns_free_message(&query);
        if (rc == 0) dns_free_message(&answer);
        return;
    }

    /* send raw cached packet */
    if (rc == 0 || rc == -3) {
        const char *qname = query.questions[0].qname;
        uint16_t qtype   = query.questions[0].qtype;
        uint16_t qclass  = query.questions[0].qclass;
        uint8_t *raw = NULL;
        size_t raw_len = 0;
        if (cache_lookup(g_cache, qname, qtype, qclass, &raw, &raw_len) == 0) {
            if (raw_len >= 2) {
                raw[0] = (query.header.id >> 8) & 0xFF;
                raw[1] = query.header.id & 0xFF;
            }
            if (g_dnssec && rc == 0) {
                dns_message_t tmp;
                if (dns_parse_message(raw, raw_len, &tmp) == 0) {
                    int sec = dnssec_validate(g_dnssec, raw, raw_len, &tmp);
                    if (sec == 0)
                        printf("DNSSEC: secure answer for %s\n", qname);
                    else if (sec == -1)
                        printf("DNSSEC: BOGUS answer for %s\n", qname);
                    dns_free_message(&tmp);
                }
            }
            response_cb(cb_arg, raw, raw_len);
        } else {
            uint8_t err_buf[12];
            dns_builder_t err;
            dns_builder_init(&err, err_buf, sizeof(err_buf));
            dns_build_response_header(&err, query.header.id,
                                      htons(DNS_QR_RESPONSE | DNS_RCODE_SERVFAIL),
                                      0, 0, 0, 0);
            response_cb(cb_arg, err_buf, dns_builder_pos(&err));
        }
    } else {
        uint8_t err_buf[12];
        dns_builder_t err;
        dns_builder_init(&err, err_buf, sizeof(err_buf));
        dns_build_response_header(&err, query.header.id,
                                  htons(DNS_QR_RESPONSE | DNS_RCODE_SERVFAIL),
                                  0, 0, 0, 0);
        response_cb(cb_arg, err_buf, dns_builder_pos(&err));
    }

    dns_free_message(&query);
    if (rc == 0 || rc == -3) dns_free_message(&answer);
}
