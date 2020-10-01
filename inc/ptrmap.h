#ifndef PTRMAP_H
#define PTRMAP_H

#include <stdint.h>
#include <assert.h>

#include "common.h"


static_assert (sizeof (intptr_t) == 64 / CHAR_BIT, "pointer bits abuse");

typedef uint64_t hash_t;
typedef void *key_t;
typedef hash_t hash_func(int len, key_t *k);

typedef struct ptrmap {
	// ]64:58] = log2(len+1)
	// ]48:4] << 4 = ptr
	intptr_t _packed;
} ptrmap;

uint8_t *pmap_meta(ptrmap m);
key_t *pmap_buf(ptrmap m);
len_t pmap_len(ptrmap m);

key_t *pmap_find(ptrmap *m, key_t *k, hash_func *fn);
key_t *pmap_push(ptrmap *m, key_t *k, hash_func *fn);
key_t *pmap_reserve(ptrmap *m, len_t n);


#endif /* PTRMAP_H */

