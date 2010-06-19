#ifndef _COMMON_H_
#define _COMMON_H_

#include "config.h"
#include "events.h"

#define APE_BASEMEM 512

typedef struct _ape_global {
	unsigned int basemem;
	struct _fdevent events;
} ape_global;


#endif
