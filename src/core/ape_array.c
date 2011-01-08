#include "common.h"
#include "ape_array.h"
#include <string.h>

ape_array_t *ape_array_new(size_t n)
{
	ape_array_t *array;
	array = (ape_array_t *)ape_new_pool_list(sizeof(ape_array_item_t), n);
	
	return array;
}


buffer *ape_array_lookup(ape_array_t *array, const char *key, int klen)
{
	buffer *k, *v;
	APE_A_FOREACH(array, k, v) {
		if (k->used == klen && memcmp(key, k->data, klen) == 0) {
			return v;
		}
	}
	
	return NULL;
}


void ape_array_add_b(ape_array_t *array, buffer *key, buffer *value)
{
	ape_array_item_t *slot = (ape_array_item_t *)array->current;
	
	if (slot == NULL || slot->pool.flags & APE_ARRAY_USED_SLOT) {
		slot = (ape_array_item_t *)ape_grow_pool(array, sizeof(ape_array_item_t), 4);
	}
	
	slot->pool.flags |= APE_ARRAY_USED_SLOT;
	
	slot->key = key;
	slot->val = value;
	
	array->current = slot->pool.next;

	if (array->current == NULL || ((ape_array_item_t *)array->current)->pool.flags & APE_ARRAY_USED_SLOT) {
		array->current = array->head;
		while (array->current != NULL && ((ape_array_item_t *)array->current)->pool.flags & APE_ARRAY_USED_SLOT) {
			array->current = ((ape_array_item_t *)array->current)->pool.next;
		}
	}
}

void ape_array_add_n(ape_array_t *array, const char *key, int klen, const char *value, int vlen)
{
	buffer *k, *v;
	
	k = buffer_new(klen+1);
	v = buffer_new(klen+1);
	
	buffer_append_string_n(k, key, klen);
	buffer_append_string_n(v, value, vlen);
	
	ape_array_add_b(array, k, v);
}

void ape_array_add(ape_array_t *array, const char *key, const char *value)
{
	ape_array_add_n(array, key, strlen(key), value, strlen(value));
}

void ape_array_destroy(ape_array_t *array)
{
	ape_destroy_pool_list_ordered((ape_pool_list_t *)array);
}

