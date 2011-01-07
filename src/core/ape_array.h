#ifndef __APE_ARRAY_H
#define __APE_ARRAY_H

#include "ape_pool.h"
#include "ape_buffer.h"

typedef ape_pool_list_t ape_array_t;

#define APE_ARRAY_FREE_SLOT (1 << 1) 

struct _ape_array_item {
	/* inherit from ape_pool_t (same first sizeof(ape_pool_t) bytes memory-print) */
	ape_pool_t pool;
	buffer *key;
	buffer *val;
} typedef ape_array_item_t;

#endif
