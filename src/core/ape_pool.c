#include "ape_pool.h"
#include <stdlib.h>

ape_pool_t *ape_new_pool(size_t size, size_t n)
{
	int i;
	ape_pool_t *pool = malloc(size * n);
	
	for (i = 0; i < n; i++) {
		/* Get the address of the current object */
		ape_pool_t *current = ((void *)&pool[0])+(i*size);
		
		current->next = (i == n-1 ? NULL : ((void *)&pool[0])+((i+1)*size)); /* contiguous blocks */
		current->ptr = NULL;
		current->flags = (i == 0 ? APE_POOL_ALLOC : 0);
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

