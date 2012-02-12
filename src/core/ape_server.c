#include "ape_server.h"
#include "JSON_parser.h"
#include "ape_transports.h"
#include "ape_modules.h"
#include "ape_base64.h"
#include "ape_sha1.h"
#include "ape_websocket.h"
#include "ape_ssl.h"

#include <string.h>

static int ape_server_http_ready(ape_client *client, ape_global *ape);

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
            (*(uint32_t *)
             path->data & 0x00FFFFFF) == *(uint32_t *)
                                            ape_transports_s[i].path) {
            
            path->pos = ape_transports_s[i].len;
            
            return ape_transports_s[i].type;
        } else if (ape_transports_s[i].len > 3 &&
            strncmp(path->data, ape_transports_s[i].path,
                ape_transports_s[i].len) == 0) {
            
            path->pos = ape_transports_s[i].len;
            
            return ape_transports_s[i].type;
        }
    }

    return APE_TRANSPORT_NU;
}

static int ape_http_callback(void **ctx, callback_type type,
        int value, uint32_t step)
{
    ape_client *client = (ape_client *)ctx[0];
    ape_global *ape = (ape_global *)ctx[1];

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
        client->http.path         = buffer_new(256);
        client->http.headers.list = ape_array_new(16);
        client->http.headers.tkey = buffer_new(16);
        client->http.headers.tval = buffer_new(64);
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
            config.depth                  = 15;
            config.callback               = NULL;
            config.callback_ctx           = NULL;
            config.allow_comments         = 0;
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
        client->http.headers.tkey   = buffer_new(16);
        client->http.headers.tval   = buffer_new(64);
        break;
    case HTTP_CL_VAL:
        break;
    case HTTP_HEADER_END:
        break;
    case HTTP_READY:
	printf("HTTP ready\n");
        ape_server_http_ready(client, ape);
        break;
    default:
        break;
    }
    return 1;
}

static void ape_server_on_ws_frame(ape_client *client, const unsigned char *data, ssize_t length, ape_global *ape)
{
	ape_ws_write(client->socket, (char *)data, length, APE_DATA_COPY);
	APE_EVENT(wsframe, client, data, length, ape);
}

static int ape_server_http_ready(ape_client *client, ape_global *ape)
{
#define REQUEST_HEADER(header) ape_array_lookup(client->http.headers.list, CONST_STR_LEN(header))

	const buffer *host = REQUEST_HEADER("host");
	const buffer *upgrade = REQUEST_HEADER("upgrade");

    if (host != NULL) {
        /* /!\ the buffer is non null terminated */
    }
	if (upgrade && strncmp(upgrade->data, CONST_STR_LEN(" websocket")) == 0) {
		char *ws_computed_key;
		const buffer *ws_key = REQUEST_HEADER("Sec-WebSocket-Key");
		printf("Key : %s\n", &ws_key->data[1]);
		if (ws_key) {
			ws_computed_key = ape_ws_compute_key(&ws_key->data[1], ws_key->used-1);
			APE_socket_write(client->socket, CONST_STR_LEN(WEBSOCKET_HARDCODED_HEADERS),APE_DATA_STATIC);
			APE_socket_write(client->socket, CONST_STR_LEN("Sec-WebSocket-Accept: "), APE_DATA_STATIC);
			APE_socket_write(client->socket, ws_computed_key, strlen(ws_computed_key), APE_DATA_STATIC);
			APE_socket_write(client->socket, CONST_STR_LEN("\r\nSec-WebSocket-Origin: 127.0.0.1\r\n\r\n"), APE_DATA_STATIC);
			client->socket->callbacks.on_read = ape_ws_process_frame;
			client->ws_state = malloc(sizeof(websocket_state));

			client->ws_state->step    = WS_STEP_START;
			client->ws_state->offset  = 0;
			client->ws_state->data    = NULL;
			client->ws_state->error   = 0;
			client->ws_state->key.pos = 0;

			client->ws_state->frame_payload.start  = 0;
			client->ws_state->frame_payload.length = 0;
			client->ws_state->frame_payload.extended_length = 0;
			client->ws_state->data_pos  = 0;
			client->ws_state->frame_pos = 0;
			client->ws_state->on_frame = ape_server_on_ws_frame;
			
			return 1;
		}
	}

    switch(client->http.transport) {
    case APE_TRANSPORT_NU:
    case APE_TRANSPORT_FT:
    {
        char fullpath[4096];
		char fill[20480*51];
		
		memset(fill, 'a', 20480*51);

        APE_EVENT(request, client, ape);
		
		printf("Got a request\n");
		
		
		APE_socket_write(client->socket, CONST_STR_LEN("HTTP/1.1 200 OK\n\n"), APE_DATA_STATIC);
		APE_socket_write(client->socket, CONST_STR_LEN("<h1>Ho heil :)</h1>\n\n"), APE_DATA_STATIC);
		APE_socket_write(client->socket, fill, 20480*51, APE_DATA_STATIC);

		//#endif
		APE_socket_shutdown(client->socket);
        
		//printf("Requested : %s\n", client->http.path->data);
		//APE_socket_write(client->socket, CONST_STR_LEN("HTTP/1.1 418 I'm a teapot\n\n"));
		//APE_sendfile(client->socket, client->http.path->data);
		//APE_socket_shutdown(client->socket);
        /*
        sprintf(fullpath, "%s%s", ((ape_server *)client->server->ctx)->chroot, client->http.path->data);
        APE_socket_write(client->socket, CONST_STR_LEN("HTTP/1.1 418 I'm a teapot\n\n"));
        APE_sendfile(client->socket, fullpath);
        APE_socket_shutdown(client->socket);
        printf("URL : %s\n", client->http.path->data);*/
        
        break;        
        
    }

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
					printf("Failed %c\n", socket_client->data_in.data[i]);
            shutdown(socket_client->s.fd, 2);
            break;
        }
    }
}

static void ape_server_on_connect(ape_socket *socket_server, ape_socket *socket_client, ape_global *ape)
{
    ape_client *client;

    client          = malloc(sizeof(*client)); /* setup the client struct */
    client->socket  = socket_client;
    client->server  = socket_server;

    socket_client->_ctx = client; /* link the socket to the client struct */

    HTTP_PARSER_RESET(&client->http.parser);

    client->http.parser.callback = ape_http_callback;
    client->http.parser.ctx[0]   = client;
    client->http.parser.ctx[1]   = ape;
    client->http.method          = HTTP_GET;
    client->http.transport       = APE_TRANSPORT_NU;
    client->http.path            = NULL;
    client->http.headers.list    = NULL;

	client->ws_state			 = NULL;

    client->json.parser = NULL;

}

static void ape_server_on_disconnect(ape_socket *socket_client, ape_global *ape)
{

    if (APE_CLIENT(socket_client)->http.path != NULL) {
        buffer_destroy(APE_CLIENT(socket_client)->http.path);
    }
    /* TODO clean headers */

    free(socket_client->_ctx); /* release the ape_client object */

    //printf("Client has disconnected\n");

} /* ape_socket object is released after this call */

ape_server *ape_server_init(ape_cfg_server_t *conf, ape_global *ape)
{
    ape_socket *socket;
    ape_server *server;
    
    uint16_t port;
    char *local_ip, *cert;
    
    port = conf->port;
    local_ip = conf->ip;
    cert = conf->SSL.cert_path;

    if ((socket = APE_socket_new((cert != NULL && conf->SSL.enabled ? APE_SOCKET_PT_SSL : APE_SOCKET_PT_TCP), 0)) == NULL ||
        APE_socket_listen(socket, port, local_ip, ape) != 0) {

        printf("[Server] Failed to initialize %s:%d\n", local_ip, port);
        APE_socket_destroy(socket, ape);
        return NULL;
    }

    server          = malloc(sizeof(*server));
    server->socket  = socket;

    if (*local_ip == '*' || *local_ip == '\0') {
        strcpy(server->ip, "0.0.0.0");
    } else {
        strncpy(server->ip, local_ip, 15);
    }
    server->ip[15]  = '\0';
    server->port    = port;

    socket->callbacks.on_read       = ape_server_on_read;
    socket->callbacks.on_connect    = ape_server_on_connect;
    socket->callbacks.on_disconnect = ape_server_on_disconnect;
    socket->_ctx                    = server; /* link the socket to the server struct */
	
	if (APE_SOCKET_ISSECURE(socket)) {
		if ((socket->SSL.ssl = ape_ssl_init_ctx(cert)) == NULL) {
		    APE_socket_destroy(socket, ape);
		    printf("[Server] Failed to start %s:%d (Failed to init SSL)\n", server->ip, server->port);
		    free(server);
		    return NULL;
		}
	}

    printf("[Server] Starting %s:%d %s\n", server->ip, server->port, (APE_SOCKET_ISSECURE(socket) ? "[SSL server]" : ""));

    return server;
}

// vim: ts=4 sts=4 sw=4 et

