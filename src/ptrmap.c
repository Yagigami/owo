#include "ptrmap.h"
#include "common.h"
#include "alloc.h"

#include <limits.h>
#include <string.h>


#define PMAP_EMPTY   0x00
#define PMAP_DELETED 0x7F
#define PMAP_FULL    0x80 //  0b1xxxxxxx

key_t *pmap_find(ptrmap *m, key_t k, hash_func *fn, cmp_func *cmp)
{
	len_t capm1 = (1 << m->log2_cap) - 1;
	hash_t hash = fn(k);
	uint8_t *meta = m->mem;
	key_t *buf = (key_t *) (meta + capm1 + 1);
	for (len_t hi = hash >> 7, lo = PMAP_FULL | (hash & 0x7F), i = hi & capm1;; i = (i + 1) & capm1) {
		if (meta[i] == PMAP_EMPTY) return NULL;
		if (meta[i] != lo) continue;
		if (cmp(buf[i], k) == 0) return buf + i;
	}
}

key_t *pmap_push(ptrmap *m, key_t k, hash_func *fn)
{
	pmap_reserve(m, m->len + 1);
	len_t capm1 = (1 << m->log2_cap) - 1;
	hash_t hash = fn(k);
	uint8_t *meta = m->mem;
	key_t *buf = (key_t *) (meta + capm1 + 1);
	for (len_t hi = hash >> 7, lo = PMAP_FULL | (hash & 0x7F), i = hi & capm1;; i = (i + 1) & capm1) {
		if (meta[i] & PMAP_FULL) continue;
		meta[i] = lo;
		buf[i] = k;
		m->len++;
		return buf + i;
	}
}

void pmap_reserve(ptrmap *m, len_t n)
{
	len_t cap = 1 << m->log2_cap;
	if (n < 8) n = 8;
	if (cap < n) {
		len_t new_cap = cap * 2;
		if (new_cap < n) new_cap = 1ll << (sizeof (n) * CHAR_BIT - 1 - __builtin_clzll(n));
		assert((new_cap & (new_cap - 1)) == 0);
		void *mem = xrealloc(m->mem, new_cap * (1 + sizeof (key_t)));
		memset(mem, PMAP_EMPTY, new_cap);
		assert(((intptr_t) mem & 0xF) == 0);
		m->mem = mem;
		m->log2_cap = __builtin_ctzll(new_cap);
	}
}

void pmap_fini(ptrmap *m)
{
	xfree(m->mem);
	memset(m, 0, sizeof *m);
}

