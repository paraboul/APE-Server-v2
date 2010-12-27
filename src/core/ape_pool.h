#ifndef __APE_POOL_H
#define __APE_POOL_H

#include "common.h"

#define APE_POOL_ALLOC 0x01
#define APE_POOL_ALL_FLAGS APE_POOL_ALLOC

typedef struct _ape_pool {
	void *ptr; /* public */
	struct _ape_pool *next;
	uint32_t flags;
} ape_pool_t;

ape_pool_t *ape_new_pool(size_t size, size_t n);
void ape_destroy_pool(ape_pool_t *pool);

#endif
