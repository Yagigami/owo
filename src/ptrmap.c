#include "ptrmap.h"
#include "common.h"
#include "alloc.h"

#include <limits.h>
#include <string.h>


#define PMAP_EMPTY   0x00
#define PMAP_DELETED 0x7F
#define PMAP_FULL    0x80 //  0b1xxxxxxx

#define PMAP_MIN_CAP 8

void pmap_init(ptrmap *m, allocator al)
{
	m->len = 0;
	m->log2_cap = 0;
	m->mem = NULL;
	m->upstream = al;
}

key_t *pmap_find(ptrmap *m, key_t k, hash_func *fn, cmp_func *cmp)
{
	len_t capm1 = (1 << m->log2_cap) - 1;
	hash_t hash = fn(k);
	uint8_t *meta = m->mem;
	key_t *buf = (key_t *) (meta + capm1 + 1);
	if (!meta) return NULL;
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
	__builtin_unreachable();
}

key_t *pmap_intern(ptrmap *m, key_t k, hash_func *fn, cmp_func *cmp)
{
	pmap_reserve(m, m->len + 1);
	len_t capm1 = (1 << m->log2_cap) - 1;
	hash_t hash = fn(k);
	uint8_t *meta = m->mem;
	key_t   *buf  = (key_t *) (meta + capm1 + 1);
	// no need for the null check in this case
	for (len_t high = hash >> 7, low = PMAP_FULL | (hash & 0x7F),
			i = high & capm1;; i = (i + 1) & capm1) {
		if ((meta[i] & PMAP_FULL) == 0) {
			meta[i] = low;
			buf [i] = k;
			m->len++;
			return buf + i;
		}
		if (meta[i] != low) continue;
		if (cmp(buf[i], k) == 0) return buf + i;
	}
	__builtin_unreachable();
}

void pmap_reserve(ptrmap *m, len_t n)
{
	len_t cap = 1 << m->log2_cap;
	if (n < PMAP_MIN_CAP) n = PMAP_MIN_CAP;
	// the map must always have at least 1 free element, so we can *not* have cap == n
	if (cap <= n) {
		len_t l2 = 64 - __builtin_clzll(n);
		len_t new_cap = 1 << l2;
		void *mem = gen_realloc(
				m->upstream,
				new_cap * (1 + sizeof (key_t)),
				m->mem,
				cap * sizeof (key_t)
		);
		assert(((intptr_t) mem & 0xF) == 0);
		memset(mem, PMAP_EMPTY, new_cap);
		m->mem = mem;
		m->log2_cap = __builtin_ctzll(new_cap);
	}
}

void pmap_fini(ptrmap *m)
{
	gen_free(m->upstream, m->mem, (1 << m->log2_cap) * PTRSZ);
#ifndef NDEBUG
	memset(m, 0, sizeof *m);
#endif
}

