#ifndef __APE_ARRAY_H
#define __APE_ARRAY_H

#include "ape_pool.h"
#include "ape_buffer.h"

typedef ape_pool_list_t ape_array_t;

#define APE_ARRAY_USED_SLOT (1 << 1) 

struct _ape_array_item {
	/* inherit from ape_pool_t (same first sizeof(ape_pool_t) bytes memory-print) */
	ape_pool_t pool;
	buffer *key;
	buffer *val;
} typedef ape_array_item_t;

ape_array_t *ape_array_new(size_t n);
buffer *ape_array_lookup(ape_array_t *array, const char *key, int klen);

void ape_array_add_b(ape_array_t *array, buffer *key, buffer *value);
void ape_array_add_n(ape_array_t *array, const char *key, int klen, const char *value, int vlen);
void ape_array_add(ape_array_t *array, const char *key, const char *value);
void ape_array_destroy(ape_array_t *array);

#define APE_A_FOREACH(_array, _key, _val) \
		ape_array_item_t *__array_item; \
		for (__array_item = (ape_array_item_t *)_array->head; __array_item != NULL; __array_item = (ape_array_item_t *)__array_item->pool.next) \
			if ((__array_item->pool.flags & APE_ARRAY_USED_SLOT) && (_key = __array_item->key) && (_val = __array_item->val)) \

#endif
