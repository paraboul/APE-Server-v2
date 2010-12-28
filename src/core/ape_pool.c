#include "ape_pool.h"
#include <stdlib.h>

ape_pool_t *ape_new_pool(size_t size, size_t n)
{
	int i;
	ape_pool_t *pool = malloc(size * n), *current = NULL;

	for (i = 0; i < n; i++) {
		current 	= ((void *)&pool[0])+(i*size);
		current->next 	= (i == n-1 ? NULL : ((void *)&pool[0])+((i+1)*size)); /* contiguous blocks */
		current->ptr 	= NULL;
		current->flags 	= (i == 0 ? APE_POOL_ALLOC : 0);
	}
	
	return pool;
}

ape_pool_list_t *ape_new_pool_list(size_t size, size_t n)
{
	ape_pool_list_t *list = malloc(sizeof(ape_pool_list_t));
	
	ape_init_pool_list(list, size, n);
	
	return list;
}

void ape_init_pool_list(ape_pool_list_t *list, size_t size, size_t n)
{
	ape_pool_t *pool = ape_new_pool(size, n);
	
	list->head 	= pool;
	list->current 	= pool;
	list->queue 	= ((void *)&pool[0])+((n-1)*size);	
}

ape_pool_t *ape_grow_pool(ape_pool_list_t *list, size_t size, size_t n)
{
	ape_pool_t *pool;
	
	pool = ape_new_pool(size, n);
	list->queue->next = pool;
	list->queue = ((void *)&pool[0])+((n-1)*size);
	
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

void ape_destroy_pool_list(ape_pool_list_t *list)
{
	ape_destroy_pool(list->head);
	free(list);
}

