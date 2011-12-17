#include "common.h"
#include "ape_extend.h"

#include <string.h>

void ape_add_property(ape_extend_t *entry, const char *key, void *val)
{
	ape_array_add_ptrn(entry, key, strlen(key), val);
}

void *ape_get_property(ape_extend_t *entry, const char *key, int klen)
{
	ape_array_item_t *item;

	if ((item = ape_array_lookup_item(entry, key, klen)) == NULL) {
	    return NULL;
	}

	return item->pool.ptr.data;
}

