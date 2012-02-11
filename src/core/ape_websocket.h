#ifndef __APE_WEBSOCKET_H
#define __APE_WEBSOCKET_H

#include "common.h"
#include "ape_socket.h"

typedef enum {
    WS_STEP_KEY,
    WS_STEP_START,
    WS_STEP_LENGTH,
    WS_STEP_SHORT_LENGTH,
    WS_STEP_EXTENDED_LENGTH,
    WS_STEP_DATA,
    WS_STEP_END
} ws_payload_step;

typedef struct _websocket_state
{
	unsigned char *data;
	void (*on_frame)(struct _ape_client *, const unsigned char *, ssize_t, ape_global *);
	
	unsigned int offset;
	unsigned short int error;
	//ws_version version;
    
	struct {
	    /* cypher key */
	    unsigned char val[4];
	    int pos;
	} key;
    
    #pragma pack(2)
	struct {
	    unsigned char start;
	    unsigned char length; /* 7 bit length */
	    union {
	        unsigned short short_length; /* 16 bit length */
	        unsigned long long int extended_length; /* 64 bit length */
	    };
	} frame_payload;
	#pragma pack()
	
	ws_payload_step step;
	int data_pos;
	int data_inkey;
	int frame_pos;
} websocket_state;

void ape_ws_process_frame(ape_socket *socket_client, ape_global *ape);
char *ape_ws_compute_key(const char *key, unsigned int key_len);
void ape_ws_write(ape_socket *socket_client, unsigned char *data,
	size_t len, ape_socket_data_autorelease data_type);

#define WEBSOCKET_HARDCODED_HEADERS "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"

#endif