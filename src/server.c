#include "server.h"


static void ape_server_on_read(ape_socket *socket_client, ape_global *ape)
{
	//printf("Data %s\n", socket_client->data_in.data);
	printf("New data\n");
}

static void ape_server_on_connect(ape_socket *socket_client, ape_global *ape)
{
	ape_client *client;
	
	client 		= malloc(sizeof(*client)); /* setup the client struct */
	client->socket	= socket_client;
	
	socket_client->ctx = client; /* link the socket to the client struct */
	
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
	
	socket->callbacks.on_read 	= ape_server_on_read;
	socket->callbacks.on_connect 	= ape_server_on_connect;
	socket->callbacks.on_disconnect = ape_server_on_disconnect;	
	
	server 		= malloc(sizeof(*server));
	server->socket 	= socket;
	
	socket->ctx 	= server; /* link the socket to the server struct */
	
	return server;
}

