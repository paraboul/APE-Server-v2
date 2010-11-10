#ifndef _COMMON_H_
#define _COMMON_H_

#include "config.h"
#include "events.h"

#define APE_BASEMEM 512
#define __REV "2.0b1"


typedef struct _ape_global {
	int basemem;
	struct _fdevent events;
} ape_global;

int events_init(ape_global *ape);

#endif
