#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <stdlib.h>
#include <sys/types.h>

typedef struct {
	char *data;
	
	size_t size;
	size_t used;
} buffer;

#endif
