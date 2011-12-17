#include "ape_server.h"
#include "JSON_parser.h"
#include "ape_transports.h"
#include "ape_modules.h"
#include "ape_base64.h"
#include "ape_sha1.h"

#include <string.h>

#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WEBSOCKET_HARDCODED_HEADERS "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"

static int ape_server_http_ready(ape_client *client, ape_global *ape);
static void ape_server_on_read_ws(ape_socket *socket_client, ape_global *ape);

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

#if 0
static void ape_process_websocket_frame(ape_socket *socket, ape_global *ape)
{
    ape_buffer *buffer = &co->buffer_in;
    websocket_state *websocket = co->parser.data;
    ape_parser *parser = &co->parser;
    
    unsigned char *pData;
    
    for (pData = (unsigned char *)&buffer->data[websocket->offset]; websocket->offset < buffer->length; websocket->offset++, pData++) {
        switch(websocket->step) {
            case WS_STEP_KEY:
                /* Copy the xor key (32 bits) */
                websocket->key.val[websocket->key.pos] = *pData;
                if (++websocket->key.pos == 4) {
                    websocket->step = WS_STEP_DATA;
                }
                break;
            case WS_STEP_START:
                /* Contain fragmentaiton infos & opcode (+ reserved bits) */
                websocket->frame_payload.start = *pData;

                websocket->step = WS_STEP_LENGTH;
                break;
            case WS_STEP_LENGTH:
                /* Check for MASK bit */
                if (!(*pData & 0x80)) {
                    return;
                }
                switch (*pData & 0x7F) { /* 7bit length */
                    case 126:
                        /* Following 16bit are length */
                        websocket->step = WS_STEP_SHORT_LENGTH;
                        break;
                    case 127:
                        /* Following 64bit are length */
                        websocket->step = WS_STEP_EXTENDED_LENGTH;
                        break;
                    default:
                        /* We have the actual length */
                        websocket->frame_payload.extended_length = *pData & 0x7F;
                        websocket->step = WS_STEP_KEY;
                        break;
                }
                break;
            case WS_STEP_SHORT_LENGTH:
                memcpy(((char *)&websocket->frame_payload)+(websocket->frame_pos), 
                        pData, 1);
                if (websocket->frame_pos == 3) {
                    websocket->frame_payload.extended_length = ntohs(websocket->frame_payload.short_length);
                    websocket->step = WS_STEP_KEY;
                }
                break;
            case WS_STEP_EXTENDED_LENGTH:
                memcpy(((char *)&websocket->frame_payload)+(websocket->frame_pos),
                        pData, 1);
                if (websocket->frame_pos == 9) {
                    websocket->frame_payload.extended_length = ntohl(websocket->frame_payload.extended_length >> 32);
                    websocket->step = WS_STEP_KEY;
                }        
                break;
            case WS_STEP_DATA:
                if (websocket->data_pos == 0) {
                    websocket->data_pos = websocket->offset;
                }
                
                *pData ^= websocket->key.val[(websocket->frame_pos - websocket->data_pos) % 4];

                if (--websocket->frame_payload.extended_length == 0) {
                    unsigned char saved;
                    
                    websocket->data = &buffer->data[websocket->data_pos];
                    websocket->step = WS_STEP_START;
                    websocket->frame_pos = -1;
                    websocket->frame_payload.extended_length = 0;
                    websocket->data_pos = 0;
                    websocket->key.pos = 0;

                    switch(websocket->frame_payload.start & 0x0F) {
                        case 0x8:
                        {
                            /*
                              Close frame
                              Reply by a close response
                            */
                            char payload_head[2] = { 0x88, 0x00 };
                            sendbin(co->fd, payload_head, 2, 0, g_ape);
                            return;
                        }
                        case 0x9:
                        {
                            int body_length = &buffer->data[websocket->offset+1] - websocket->data;
                            char payload_head[2] = { 0x8a, body_length & 0x7F };
                            
                            /* All control frames MUST be 125 bytes or less */
                            if (body_length > 125) {
                                payload_head[0] = 0x88;
                                payload_head[1] = 0x00;      
                                sendbin(co->fd, payload_head, 2, 1, g_ape);
                                return;
                            }
                            PACK_TCP(co->fd);
                            sendbin(co->fd, payload_head, 2, 0, g_ape);
                            if (body_length) {
                                sendbin(co->fd, websocket->data, body_length, 0, g_ape);
                            }
                            FLUSH_TCP(co->fd);
                            break;
                        }
                        case 0xA: /* Never called as long as we never ask for pong */
                            break;
                        default:
                            /* Data frame */
                            saved = buffer->data[websocket->offset+1];
                            buffer->data[websocket->offset+1] = '\0';
                            parser->onready(parser, g_ape);
                            buffer->data[websocket->offset+1] = saved;                            
                            break;
                    }
                    
                    if (websocket->offset+1 == buffer->length) {
                        websocket->offset = 0;
                        buffer->length = 0;
                        websocket->frame_pos = 0;
                        websocket->key.pos = 0;
                        return;
                    }
                }
                break;
            default:
                break;
        }
        websocket->frame_pos++;
    }
}
#endif

static char *ape_ws_compute_key(const char *key, unsigned int key_len)
{
    unsigned char digest[20];
    char out[128];
    char *b64;
    
    if (key_len > 32) {
        return NULL;
    }
    
    memcpy(out, key, key_len);
    memcpy(out+key_len, WS_GUID, sizeof(WS_GUID)-1);
    
    sha1_csum((unsigned char *)out, (sizeof(WS_GUID)-1)+key_len, digest);
    
    b64 = base64_encode(digest, 20);
    
    return b64; /* must be released */
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
			APE_socket_write(client->socket, CONST_STR_LEN(WEBSOCKET_HARDCODED_HEADERS));
			APE_socket_write(client->socket, CONST_STR_LEN("Sec-WebSocket-Accept: "));
			APE_socket_write(client->socket, ws_computed_key, strlen(ws_computed_key));
			APE_socket_write(client->socket, CONST_STR_LEN("\r\nSec-WebSocket-Origin: 127.0.0.1\r\n"));
			APE_socket_write(client->socket, CONST_STR_LEN("\r\n\r\n"));
			client->socket->callbacks.on_read = ape_server_on_read_ws;
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
			
			
			return 1;
		}
	}

    switch(client->http.transport) {
    case APE_TRANSPORT_NU:
    case APE_TRANSPORT_FT:
    {
        char fullpath[4096];

        APE_EVENT(request, client, ape);
        
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

static void ape_server_on_read_ws(ape_socket *socket_client, ape_global *ape)
{
	const buffer *buffer = &socket_client->data_in;
	unsigned char *pData;
	printf("Reading...WS %d\n", socket_client->data_in.used);
	
	#if 1
	websocket_state *websocket = APE_CLIENT(socket_client)->ws_state;
	
	printf("Starting at offset : %d\n", websocket->offset);
	
    for (pData = (unsigned char *)&buffer->data[websocket->offset]; websocket->offset < buffer->used; websocket->offset++, pData++) {
        switch(websocket->step) {
            case WS_STEP_KEY:
                /* Copy the xor key (32 bits) */
                websocket->key.val[websocket->key.pos] = *pData;
				printf("Reading key\n");
                if (++websocket->key.pos == 4) {
					printf("Key read\n");
                    websocket->step = WS_STEP_DATA;
                }
                break;
            case WS_STEP_START:
                /* Contain fragmentaiton infos & opcode (+ reserved bits) */
                websocket->frame_payload.start = *pData;

                websocket->step = WS_STEP_LENGTH;
                break;
            case WS_STEP_LENGTH:
                /* Check for MASK bit */
                if (!(*pData & 0x80)) {
                    return;
                }
                switch (*pData & 0x7F) { /* 7bit length */
                    case 126:
                        /* Following 16bit are length */
                        websocket->step = WS_STEP_SHORT_LENGTH;
                        break;
                    case 127:
                        /* Following 64bit are length */
                        websocket->step = WS_STEP_EXTENDED_LENGTH;
                        break;
                    default:
                        /* We have the actual length */
                        websocket->frame_payload.extended_length = *pData & 0x7F;
                        websocket->step = WS_STEP_KEY;
                        break;
                }
                break;
            case WS_STEP_SHORT_LENGTH:
                memcpy(((char *)&websocket->frame_payload)+(websocket->frame_pos), 
                        pData, 1);
                if (websocket->frame_pos == 3) {
                    websocket->frame_payload.extended_length = ntohs(websocket->frame_payload.short_length);
                    websocket->step = WS_STEP_KEY;
                }
                break;
            case WS_STEP_EXTENDED_LENGTH:
                memcpy(((char *)&websocket->frame_payload)+(websocket->frame_pos),
                        pData, 1);
                if (websocket->frame_pos == 9) {
                    websocket->frame_payload.extended_length = ntohl(websocket->frame_payload.extended_length >> 32);
                    websocket->step = WS_STEP_KEY;
                }        
                break;
            case WS_STEP_DATA:
                if (websocket->data_pos == 0) {
                    websocket->data_pos = websocket->offset;
                }
                
                *pData ^= websocket->key.val[(websocket->frame_pos - websocket->data_pos) % 4];
				printf("Val : %c\n", *pData);
                if (--websocket->frame_payload.extended_length == 0) {
                    unsigned char saved;
                    
                    websocket->data = &buffer->data[websocket->data_pos];
                    websocket->step = WS_STEP_START;
                    websocket->frame_pos = -1;
                    websocket->frame_payload.extended_length = 0;
                    websocket->data_pos = 0;
                    websocket->key.pos = 0;

                    switch(websocket->frame_payload.start & 0x0F) {
                        case 0x8:
                        {
                            /*
                              Close frame
                              Reply by a close response
                            */
                            char payload_head[2] = { 0x88, 0x00 };
                            //sendbin(co->fd, payload_head, 2, 0, g_ape);
                            return;
                        }
                        case 0x9:
                        {
                            int body_length = &buffer->data[websocket->offset+1] - websocket->data;
                            char payload_head[2] = { 0x8a, body_length & 0x7F };
                            
                            /* All control frames MUST be 125 bytes or less */
                            if (body_length > 125) {
                                payload_head[0] = 0x88;
                                payload_head[1] = 0x00;      
                            //    sendbin(co->fd, payload_head, 2, 1, g_ape);
                                return;
                            }
                            PACK_TCP(co->fd);
                          //  sendbin(co->fd, payload_head, 2, 0, g_ape);
                            if (body_length) {
                                //sendbin(co->fd, websocket->data, body_length, 0, g_ape);
                            }
                            FLUSH_TCP(co->fd);
                            break;
                        }
                        case 0xA: /* Never called as long as we never ask for pong */
                            break;
                        default:
                            /* Data frame */
                            saved = buffer->data[websocket->offset+1];
                            buffer->data[websocket->offset+1] = '\0';
                            //parser->onready(parser, g_ape);
                            buffer->data[websocket->offset+1] = saved;                            
                            break;
                    }
                    
                    if (websocket->offset+1 == buffer->used) {
                        websocket->offset = 0;
                        //buffer->length = 0;
                        websocket->frame_pos = 0;
                        websocket->key.pos = 0;
                        return;
                    }
                }
                break;
            default:
                break;
        }
        websocket->frame_pos++;
    }
#endif
}

static void ape_server_on_read(ape_socket *socket_client, ape_global *ape)
{
    int i;

    /* TODO : implement duff device here (speedup !)*/
    for (i = 0; i < socket_client->data_in.used; i++) {
		printf("%c", socket_client->data_in.data[i]);
        if (!parse_http_char(&APE_CLIENT(socket_client)->http.parser,
                socket_client->data_in.data[i])) {
					printf("\n");
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

ape_server *ape_server_init(uint16_t port, const char *local_ip, ape_global *ape)
{
    ape_socket *socket;
    ape_server *server;

    if ((socket = APE_socket_new(APE_SOCKET_PT_TCP, 0)) == NULL ||
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

    printf("[Server] Starting %s:%d\n", server->ip, server->port);

    return server;
}

// vim: ts=4 sts=4 sw=4 et

