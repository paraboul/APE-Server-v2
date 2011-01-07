#include "common.h"
#include "ape_array.h"


ape_array_t *ape_array_new(size_t size)
{
	ape_array_t *array;
	array = (ape_array_t *)ape_new_pool_list(size, sizeof(ape_array_item_t));
	
	array->queue = array->head;
	
	return array;
}

buffer *ape_array_lookup(ape_array_t *array, const char *key, int klen)
{

}

void ape_array_add_n(ape_array_t *array, const char *key, int klen, const char *value, int vlen)
{
	
}

void ape_array_add(ape_array_t *array, const char *key, const char *value)
{
	ape_array_add_n(array, key, strlen(key), value, strlen(value));
}

void ape_array_destroy(ape_array_t *)
{

}
