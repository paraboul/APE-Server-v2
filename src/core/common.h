#ifndef _APE_COMMON_H_
#define _APE_COMMON_H_

#include <stdio.h>
#include <c-ares/ares.h>


#define APE_BASEMEM 40960
#define __REV "2.0wip"

#define ape_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))
#define ape_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))

#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
#define CONST_STR_LEN2(x) x ? sizeof(x) - 1 : 0, x

#define _APE_ABS_MASK(val) (val >> sizeof(int) * 8 - 1)
#define APE_ABS(val) (val + _APE_ABS_MASK(val)) ^ _APE_ABS_MASK(val)

//#define _HAVE_MSGPACK

typedef struct _ape_global ape_global;

#include "ape_config.h"
#include "ape_events.h"
#include "ape_hash.h"
#include "ape_extend.h"

unsigned int _ape_seed;
struct _ape_client;

typedef struct _ape_module {
    char *name;
    int (*ape_module_init)(ape_global *);
    int (*ape_module_loaded)(ape_global *);
    int (*ape_module_request)(struct _ape_client *, ape_global *);
	int (*ape_module_wsframe)(struct _ape_client *, const unsigned char*, ssize_t, ape_global *);
    int (*ape_module_destroy)(ape_global *);
} ape_module_t;

extern ape_module_t  *ape_modules[];

struct _ape_global {
    int basemem;
    void *ctx; /* public */
    cfg_t *conf;

    unsigned int seed;
    struct _fdevent events;

    struct {
        ares_channel channel;
        struct {
            struct _ares_sockets *list;
            size_t size;
            size_t used;
        } sockets;

    } dns;

    struct {
        ape_htable_t *servers;
        ape_htable_t *cmds;
        
        struct {
            ape_htable_t *pub;
            ape_htable_t *priv;
        } pipes;
        
    } hashs;
    
	struct {
		struct _ticks_callback *timers;
		unsigned int ntimers;
	} timers;
	
    int is_running;
    ape_extend_t *extend;
};

typedef enum {
    APE_CLIENT_SERIAL_JSON,
#ifdef _HAVE_MSGPACK
    APE_CLIENT_SERIAL_MSGPACK
#endif
} ape_client_serial_method_e;


#endif

// vim: ts=4 sts=4 sw=4 et

