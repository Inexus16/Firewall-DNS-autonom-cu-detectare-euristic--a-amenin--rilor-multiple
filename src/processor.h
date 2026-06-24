#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>
#include <stddef.h>
#include <netinet/in.h>

/* Structura pentru callback‑ul UDP  */
struct udp_cb_arg {
    int sock;
    struct sockaddr_in client;
    socklen_t client_len;
};

/* Callback type for sending a raw DNS response */
typedef void (*response_callback_t)(void *arg, const uint8_t *data, size_t len);

/* UDP and TCP callback functions (exported for use in main.c) */
void udp_response_cb(void *arg, const uint8_t *data, size_t len);
void tcp_response_cb(void *arg, const uint8_t *data, size_t len);


void process_dns_query(
    uint8_t *inbuf, size_t inbuf_len,
    struct sockaddr_in *client, socklen_t *client_len,
    response_callback_t cb, void *cb_arg);

#endif 
