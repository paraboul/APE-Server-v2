#ifndef _COMMON_H_
#define _COMMON_H_

#include "config.h"
#include <stdio.h>

#define APE_BASEMEM 512
#define __REV "2.0wip"

typedef struct _ape_global ape_global;

#include "events.h"


struct _ape_global {
	int basemem;
	struct _fdevent events;
	int is_running:1;
};


#endif
