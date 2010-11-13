#include "server.h"
#include "http_parser.h"

static int ape_http_callback(void *ctx, callback_type type, int value, uint32_t step)
{
	ape_client *client = (ape_client *)ctx;
	
	switch(type) {
		case HTTP_METHOD:
			switch(value) {
				case HTTP_GET:
					client->http.method = HTTP_GET;
					break;
				case HTTP_POST:
					client->http.method = HTTP_POST;
					break;
			}
			client->http.path = buffer_new(128);
			break;
		case HTTP_PATH_END:
			buffer_append_char(client->http.path, '\0');
			printf("Path : %s\n", client->http.path->data);
			break;
		case HTTP_PATH_CHAR:
			buffer_append_char(client->http.path, (unsigned char)value);
			break;
		case HTTP_VERSION_MAJOR:
		case HTTP_VERSION_MINOR:
			printf("Version detected %i\n", value);
			break;
		case HTTP_HEADER_KEY:
			printf("Header key\n");
			break;
		case HTTP_HEADER_VAL:
			printf("Header value\n");
			break;
		case HTTP_CL_VAL:
			printf("CL value : %i\n", value);
			break;
		case HTTP_HEADER_END:
			printf("--------- HEADERS END ---------\n");
			ape_socket_write_file(client->socket, client->http.path->data, NULL);
			break;
		default:
			break;
	}
	return 1;
}

static void ape_server_on_read(ape_socket *socket_client, ape_global *ape)
{
	int i;
	
	/* TODO : implement duff device here (speedup !)*/
	for (i = 0; i < socket_client->data_in.used; i++) {
		if (!parse_http_char(&APE_CLIENT(socket_client)->http.parser, socket_client->data_in.data[i])) {
			shutdown(socket_client->fd, 2);
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
	
	HTTP_PARSER_RESET(&client->http.parser);
	
	client->http.parser.callback 	= ape_http_callback;
	client->http.parser.ctx 	= client;
	client->http.method		= HTTP_GET;
	
	client->http.path		= NULL;
	
	printf("New client\n");
}

static void ape_server_on_disconnect(ape_socket *socket_client, ape_global *ape)
{
	if (APE_CLIENT(socket_client)->http.path != NULL) {
		buffer_destroy(APE_CLIENT(socket_client)->http.path);
	}
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

