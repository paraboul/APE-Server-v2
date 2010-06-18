#ifndef _COMMON_H_
#define _COMMON_H_

#include "events.h"

#define APE_BASEMEM 512

#define USE_EPOLL_HANDLER 1

typedef struct _ape_global {
	unsigned int basemem;
	struct _fdevent events;
} ape_global;


#endif
