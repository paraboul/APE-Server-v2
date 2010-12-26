#include "ape_pool.h"
#include <stdlib.h>

ape_pool_t *ape_new_pool(size_t n)
{
	int i;
	ape_pool_t *pool = malloc(sizeof(*pool) * n);
	
	for (i = 0; i < n; i++) {
		pool[i].next = (i == n-1 ? NULL : &pool[i+1]); /* contiguous blocks */
		pool[i].ptr = NULL;
		pool[i].flags = (i == 0 ? APE_POOL_ALLOC : 0);
	}
	
	return pool;
}

void ape_destroy_pool(ape_pool_t *pool)
{
	ape_pool_t *tPool = NULL;
	
	while (pool != NULL) {
		/* TODO : callback ? (cleaner) */
		if (pool->flags & APE_POOL_ALLOC) {
			if (tPool != NULL) {
				free(tPool);
			}
			tPool = pool;
		}
		pool = pool->next;
	}
	if (tPool != NULL) {
		free(tPool);
	}
}

