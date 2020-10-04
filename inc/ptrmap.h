#ifndef PTRMAP_H
#define PTRMAP_H

#include <stdint.h>
#include <assert.h>

#include "common.h"


static_assert (sizeof (intptr_t) == 64 / CHAR_BIT, "pointer bits abuse");

typedef uint64_t hash_t;
typedef void *key_t;
typedef hash_t hash_func(key_t *k);

typedef struct ptrmap {
	void *mem;
	len_t len: 56;
	len_t log2_cap: 8;
} ptrmap;

key_t *pmap_find(ptrmap *m, key_t *k, hash_func *fn);
key_t *pmap_push(ptrmap *m, key_t *k, hash_func *fn);
void pmap_reserve(ptrmap *m, len_t n);


#endif /* PTRMAP_H */

