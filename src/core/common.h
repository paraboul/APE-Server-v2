#ifndef _APE_COMMON_H_
#define _APE_COMMON_H_

#include "config.h"

#include <stdio.h>
#include <c-ares/ares.h>


#define APE_BASEMEM 4096
#define __REV "2.0wip"

#define ape_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))
#define ape_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))

typedef struct _ape_global ape_global;

#include "ape_config.h"
#include "events.h"


typedef struct _ape_module {
	char *name;
	int (*ape_module_init)(ape_global *);
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

	int is_running;
};


#endif
