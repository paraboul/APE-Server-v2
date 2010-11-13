#ifndef __SERVER_H
#define __SERVER_H

#include "common.h"
#include "socket.h"

typedef struct {
	ape_socket *socket;
	char ip[16];
} ape_client;

typedef struct {
	ape_socket *socket;  /* socket of the server   */
	uint32_t nconnected; /* number of fd connected */
} ape_server;

ape_server *ape_server_init(uint16_t port, const char *local_ip, ape_global *ape);

#endif
