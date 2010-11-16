#ifndef __APE_HASH_H
#define __APE_HASH_H

#include <stdint.h>

uint64_t hash(const void * key, int len, unsigned int seed);
uint64_t uniqid(const char *seed_key, int len);

#endif
