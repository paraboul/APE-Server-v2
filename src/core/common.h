#ifndef _APE_COMMON_H_
#define _APE_COMMON_H_

#include "config.h"

#include <stdio.h>
#include <c-ares/ares.h>


#define APE_BASEMEM 4096
#define __REV "2.0wip"

#define ape_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))
#define ape_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
#define CONST_STR_LEN2(x) x ? sizeof(x) - 1 : 0, x

#define _APE_ABS_MASK(val) (val >> sizeof(int) * 8 - 1)
#define APE_ABS(val) (val + _APE_ABS_MASK(val)) ^ _APE_ABS_MASK(val)

typedef struct _ape_global ape_global;

#include "ape_config.h"
#include "ape_events.h"
#include "ape_hash.h"

unsigned int _ape_seed;

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
	
	struct {
		ape_htable_t *servers;
	} hashs;

	int is_running;
};


#endif
