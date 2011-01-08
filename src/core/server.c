#include "server.h"
#include "http_parser.h"
#include "JSON_parser.h"
#include "ape_transports.h"

static int ape_server_http_ready(ape_client *client);

static struct _ape_transports_s {
	ape_transport_t type;
	size_t len;
	char path[12];
} ape_transports_s[] = {
	{APE_TRANSPORT_LP, CONST_STR_LEN2("/1/")},
	{APE_TRANSPORT_WS, CONST_STR_LEN2("/2/")},
	{APE_TRANSPORT_FT, CONST_STR_LEN2(APE_STATIC_URI)},
	{APE_TRANSPORT_NU, CONST_STR_LEN2("")}
};

static ape_transport_t ape_get_transport(buffer *path)
{
	int i;
	
	if (path->data == NULL || *path->data == '\0' || *path->data != '/') {
		return APE_TRANSPORT_NU;
	}
	
	for (i = 0; ape_transports_s[i].type != APE_TRANSPORT_NU; i++) {
		if (path->used - 1 < ape_transports_s[i].len) continue;
		
		if (ape_transports_s[i].len == 3 &&
			(*(uint32_t *)path->data & 0x00FFFFFF) == *(uint32_t *)ape_transports_s[i].path) {

			return ape_transports_s[i].type;
		} else if (ape_transports_s[i].len > 3 &&
			strncmp(path->data, ape_transports_s[i].path, ape_transports_s[i].len) == 0) {
			
			return ape_transports_s[i].type;
		}
	}
	
	return APE_TRANSPORT_NU;
}

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
		client->http.path 		= buffer_new(32);
		client->http.headers.list 	= ape_array_new(12);
		client->http.headers.tkey	= buffer_new(16);
		client->http.headers.tval	= buffer_new(64);
		break;
	case HTTP_PATH_CHAR:
		buffer_append_char(client->http.path, (unsigned char)value);
		break;
	case HTTP_QS_CHAR:
		if (client->http.method == HTTP_GET && 
			APE_TRANSPORT_QS_ISJSON(client->http.transport) && 
			client->json.parser != NULL) {
			
			if (!JSON_parser_char(client->json.parser, (unsigned char)value)) {
				printf("Bad JSON\n");
			}
		} else {
			
			/* bufferize */
		}
		break;
	case HTTP_HEADER_KEYC:
		buffer_append_char(client->http.headers.tkey, (unsigned char)value);
		break;
	case HTTP_HEADER_VALC:
		buffer_append_char(client->http.headers.tval, (unsigned char)value);
		break;
	case HTTP_BODY_CHAR:
		if (APE_TRANSPORT_QS_ISJSON(client->http.transport) && 
			client->json.parser != NULL) {
			if (!JSON_parser_char(client->json.parser, (unsigned char)value)) {
				printf("Bad JSON\n");
			}
		} 
		break;
	case HTTP_PATH_END:
		buffer_append_char(client->http.path, '\0');
		client->http.transport = ape_get_transport(client->http.path);
		
		if (APE_TRANSPORT_QS_ISJSON(client->http.transport)) {
			JSON_config config;
			init_JSON_config(&config);
			config.depth		= 15;
			config.callback		= NULL;
			config.callback_ctx	= NULL;
			config.allow_comments	= 0;
			config.handle_floats_manually = 0;
		
			client->json.parser = new_JSON_parser(&config);				
		}
		break;
	case HTTP_VERSION_MINOR:	
		/* fall through */
	case HTTP_VERSION_MAJOR:
		break;
	case HTTP_HEADER_KEY:
		break;
	case HTTP_HEADER_VAL:
		ape_array_add_b(client->http.headers.list, 
				client->http.headers.tkey, client->http.headers.tval);
		client->http.headers.tkey	= buffer_new(16);
		client->http.headers.tval	= buffer_new(64);
		break;
	case HTTP_CL_VAL:
		break;
	case HTTP_HEADER_END:
		break;
	case HTTP_READY:
		ape_server_http_ready(client);
		break;
	default:
		break;
	}
	return 1;
}

static int ape_server_http_ready(ape_client *client)
{
	const buffer *host = ape_array_lookup(client->http.headers.list, 
			CONST_STR_LEN("host"));
	
	if (host != NULL) {
		/* /!\ the buffer is non null terminated */
		
	}
	
	switch(client->http.transport) {
	case APE_TRANSPORT_FT:
		printf("FT detected\n");
		break;
	default:
		break;
	}
	
	return 0;
}

static void ape_server_on_read(ape_socket *socket_client, ape_global *ape)
{
	int i;
	
	/* TODO : implement duff device here (speedup !)*/
	for (i = 0; i < socket_client->data_in.used; i++) {
		if (!parse_http_char(&APE_CLIENT(socket_client)->http.parser, 
				socket_client->data_in.data[i])) {
			shutdown(socket_client->s.fd, 2);
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
	client->http.transport		= APE_TRANSPORT_NU;
	client->http.path		= NULL;
	client->http.headers.list	= NULL;
	
	client->json.parser = NULL;

}

static void ape_server_on_disconnect(ape_socket *socket_client, ape_global *ape)
{
	if (APE_CLIENT(socket_client)->http.path != NULL) {
		buffer_destroy(APE_CLIENT(socket_client)->http.path);
	}
	/* TODO clean headers */
	
	free(socket_client->ctx); /* release the ape_client object */
	
	//printf("Client has disconnected\n");
	
} /* ape_socket object is released after this call */

ape_server *ape_server_init(uint16_t port, const char *local_ip, ape_global *ape)
{
	ape_socket *socket;
	ape_server *server;
	
	if ((socket = APE_socket_new(APE_SOCKET_PT_TCP, 0)) == NULL ||
		APE_socket_listen(socket, port, local_ip, ape) != 0) {
		APE_socket_destroy(socket, ape);
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

