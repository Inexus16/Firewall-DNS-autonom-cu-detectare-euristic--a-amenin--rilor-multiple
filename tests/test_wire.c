#include <stdio.h>
#include <string.h>
#include "../src/wire.h"

/* Pre-built DNS query for google.com A */
static const uint8_t query_google_com[] = {
    0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 'g', 'o', 'o', 'g', 'l', 'e', 0x03, 'c', 'o', 'm', 0x00,
    0x00, 0x01, 0x00, 0x01
};

int main() {
    dns_message_t msg;
    if (dns_parse_message(query_google_com, sizeof(query_google_com), &msg) < 0) {
        printf("FAIL: parse error\n");
        return 1;
    }
    printf("Parsed message:\n");
    dns_print_message(&msg);
    if (msg.qdcount != 1 || msg.questions[0].qtype != 1) {
        printf("FAIL: unexpected question count or type\n");
        dns_free_message(&msg);
        return 1;
    }
    printf("PASS\n");
    dns_free_message(&msg);

    /* Test building and re-parsing */
    uint8_t buf[512];
    dns_builder_t b;
    dns_builder_init(&b, buf, sizeof(buf));
    dns_build_response_header(&b, 0x1234, 0x8180, 1, 1, 0, 0);
    dns_build_question(&b, "google.com.", 1, 1);
    uint8_t ip[4] = {1, 2, 3, 4};
    dns_build_rr(&b, "google.com.", 1, 1, 300, ip, 4);

    dns_message_t parsed;
    if (dns_parse_message(buf, dns_builder_pos(&b), &parsed) < 0) {
        printf("FAIL: re-parse error\n");
        return 1;
    }
    printf("Re-parsed built message:\n");
    dns_print_message(&parsed);
    dns_free_message(&parsed);

    return 0;
}
