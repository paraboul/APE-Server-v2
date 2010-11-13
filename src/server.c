#include "server.h"
#include "http_parser.h"

static int ape_http_callback(void *ctx, callback_type type, int value, uint32_t step)
{
	ape_client *client = (ape_client *)ctx;
	
	printf("We got something\n");
}

static void ape_server_on_read(ape_socket *socket_client, ape_global *ape)
{
	int i;
	
	for (i = 0; i < socket_client->data_in.used; i++) {
		if (!parse_http_char(&APE_CLIENT(socket_client)->http, socket_client->data_in.data[i])) {
			printf("Failed to parse http\n");
			break;
		}
	}
}

static void ape_server_on_connect(ape_socket *socket_client, ape_global *ape)
{
	ape_client *client;
	
	client 		= malloc(sizeof(*client)); /* setup the client struct */
	client->socket	= socket_client;
	
	socket_client->ctx = client; /* link the socket to the client struct */
	
	HTTP_PARSER_RESET(&client->http);
	
	client->http.callback 	= ape_http_callback;
	client->http.ctx 	= client;
	
	printf("New client\n");
}

static void ape_server_on_disconnect(ape_socket *socket_client, ape_global *ape)
{
	free(socket_client->ctx); /* release the ape_client object */
	
	printf("Client has disconnected\n");
	
} /* ape_socket object is released after this call */

ape_server *ape_server_init(uint16_t port, const char *local_ip, ape_global *ape)
{
	ape_socket *socket;
	ape_server *server;
	
	if ((socket = APE_socket_new(APE_SOCKET_TCP, 0)) == NULL ||
		APE_socket_listen(socket, port, local_ip, ape) == 0) {
		
		return NULL;
	}
	
	server 		= malloc(sizeof(*server));
	server->socket 	= socket;
		
	socket->callbacks.on_read 	= ape_server_on_read;
	socket->callbacks.on_connect 	= ape_server_on_connect;
	socket->callbacks.on_disconnect = ape_server_on_disconnect;	
	socket->ctx 			= server; /* link the socket to the server struct */
	
	return server;
}

