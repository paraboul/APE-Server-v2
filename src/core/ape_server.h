#ifndef __APE_SERVER_H
#define __APE_SERVER_H

#include "common.h"
#include "ape_socket.h"
#include "ape_http_parser.h"
#include "JSON_parser.h"
#include "ape_buffer.h"
#include "ape_array.h"
#include "ape_transports.h"

#define SSL_

#define APE_CLIENT(socket) ((ape_client *)socket->_ctx)

typedef struct _ape_client {
    char ip[16];
    ape_socket *socket;
    ape_socket *server;
    struct _websocket_state *ws_state;

    struct {
        http_parser parser;
        http_method_t method;
        ape_transport_t transport;
        buffer *path;
        buffer *qs;
        struct {
            ape_array_t *list;
            buffer *tkey;
            buffer *tval;
        } headers;
    } http;

    struct {
        struct JSON_parser_struct* parser;
    } json;

} ape_client;

typedef struct {
    char ip[16];
    char *chroot;
    ape_array_t *hosts;
    ape_socket *socket;  /* socket of the server   */
    uint16_t port;
    uint32_t nconnected; /* number of fd connected */
} ape_server;

typedef struct _ape_server_conf {

} ape_server_conf_t;

ape_server *ape_server_init(uint16_t port, const char *local_ip,
        char *cert, ape_global *ape);

#endif

// vim: ts=4 sts=4 sw=4 et

