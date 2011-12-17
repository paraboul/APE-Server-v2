#ifndef __APE_EXTEND_H
#define __APE_EXTEND_H

#include "ape_array.h"

typedef ape_array_t ape_extend_t;

void ape_add_property(ape_extend_t *entry, const char *key, void *val);
void *ape_get_property(ape_extend_t *entry, const char *key, int klen);

#endif

