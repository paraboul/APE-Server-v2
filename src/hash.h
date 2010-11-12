#ifndef __HASH_H
#define __HASH_H

#include <stdint.h>

uint64_t hash(const void * key, int len, unsigned int seed);

#endif
