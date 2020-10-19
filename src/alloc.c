#include "common.h"
#include "alloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdalign.h>
#include <assert.h>


alloc_base system_allocator = ALLOC_DEFAULT;

void *xmalloc(len_t s)
{
	void *mem = malloc(s);
	if (mem) return mem;
	fprintf(stderr, "xmalloc(%zd) failed.\n", s);
	exit(1);
}

void *xrealloc(void *p, len_t s)
{
	void *mem = realloc(p, s);
	if (mem) return mem;
	fprintf(stderr, "xrealloc(%p, %zd) failed.\n", p, s);
	exit(1);
}

void *xcalloc(len_t s, len_t n)
{
	void *mem = calloc(s, n);
	if (mem) return mem;
	fprintf(stderr, "xcalloc(%zd, %zd) failed.\n", s, n);
	exit(1);
}

void xfree(void *mem)
{
	free(mem);
}

void arena_init(mem_arena *ar, len_t sz)
{
	assert(ISPOW2(sz) && sz >= 16);
	char *start = xmalloc(sz);
	ar->cur = start + PTRSZ; // let space for a `next` pointer
	*(void **) start = NULL;
	ar->end = start + sz;
	ar->_packed = ((intptr_t) ALLOC_ARENA << 56) | (intptr_t) start;
}

void arena_fini(mem_arena *ar)
{
	void *cur = (void *) (ar->_packed & BITS(56));
	do {
		void *next = *(void **) cur;
		xfree(cur);
		cur = next;
	} while (cur);
}

void *arena_alloc(mem_arena *ar, len_t sz)
{
	return arena_alloc_align(ar, sz, alignof (void *));
}

void *arena_alloc_align(mem_arena *ar, len_t sz, len_t align)
{
	assert(ISPOW2(align));
	char *new = (char *) ALIGN_UP_P2((intptr_t) ar->cur, align);
	if (new + sz >= ar->end) {
		char *start = (void *) (ar->_packed & BITS(56));
		len_t blk_sz = 2 * (ar->end - start);
		assert(blk_sz > (len_t) PTRSZ + sz + align);
		void *blk = xmalloc(blk_sz);
		*(void **) blk = start;
		start = blk;
		ar->end = start + blk_sz;
		ar->cur = start + PTRSZ;
		ar->_packed = ((intptr_t) ALLOC_ARENA << 56) | (intptr_t) start;
		new = (char *) ALIGN_UP_P2((intptr_t) ar->cur, align);
	}
	ar->cur = new + sz;
	return new;
}

static void *pool_ptr(const mem_pool *p)
{
	return (void *) (p->_packed & BITRANGE(3, 48));
}

static len_t pool_objsz(const mem_pool *p)
{
	return 1 << (p->_packed & 0x7);
}

static void pool_set_ptr(mem_pool *p, void *ptr)
{
	assert(((intptr_t) ptr & 0x7) == 0);
	intptr_t mask = BITRANGE(3, 48);
	p->_packed = ((intptr_t) ptr & mask) | (p->_packed & ~mask);
}

void pool_init(mem_pool *p, len_t sz, len_t objsz)
{
	assert(objsz >= 8);
	arena_init(&p->ar, sz);
	p->_packed = ((intptr_t) ALLOC_FIXED_POOL << 56) | (63 - __builtin_clzll(objsz));
}

void pool_fini(mem_pool *p)
{
	arena_fini(&p->ar);
}

void *pool_alloc(mem_pool *p)
{
	void *mem = pool_ptr(p);
	if (mem) {
		pool_set_ptr(p, *(void **) mem);
		return mem;
	}
	return arena_alloc(&p->ar, pool_objsz(p));
}

void pool_free(mem_pool *p, void *mem)
{
	assert(mem);
	*(void **) mem = pool_ptr(p);
	pool_set_ptr(p, mem);
}

void mp_init(multipool *mp, len_t minsz, len_t maxsz, len_t blksz)
{
	mp->base = ALLOC_MULTI_POOL;
	mp->buf = 0;
	for (len_t sz = minsz; sz <= maxsz; sz *= 2) {
		mem_pool p;
		pool_init(&p, blksz, sz);
		sm_add(&system_allocator, &mp->buf, &p, sizeof p);
	}
}

void mp_fini(multipool *mp)
{
	mem_pool *pools = sm_mem(mp->buf);
	len_t len = sm_len(mp->buf);
	for (len_t i = 0; i < len; i++) {
		pool_fini(pools + i);
	}
	xfree(pools);
}

static len_t mp_index(const multipool *mp, len_t sz)
{
	len_t l2 = 63 - __builtin_clzll(sz);
	const mem_pool *pools = sm_mem(mp->buf);
	return l2 - (pools[0]._packed & 0x7);
}

void *mp_alloc(multipool *mp, len_t sz)
{
	mem_pool *pools = sm_mem(mp->buf);
	return pool_alloc(pools + mp_index(mp, sz));
}

void mp_free(multipool *mp, void *mem, len_t sz)
{
	mem_pool *pools = sm_mem(mp->buf);
	pool_free(pools + mp_index(mp, sz), mem);
}

// can be overridden if you want more allocators
__attribute__((weak))
void *gen_alloc(allocator al, len_t sz)
{
	alloc_base base = *(int8_t *) al;
	switch (base) {
	case ALLOC_DEFAULT:
		return xmalloc(sz);
	case ALLOC_ARENA:
		return arena_alloc(al, sz);
	case ALLOC_FIXED_POOL:
		return pool_alloc(al);
	case ALLOC_MULTI_POOL:
		return mp_alloc(al, sz);
	default:
		__builtin_unreachable();
	}
}
__attribute__((weak))
void gen_free(allocator al, void *mem, len_t sz)
{
	alloc_base base = *(int8_t *) al;
	switch (base) {
		case ALLOC_DEFAULT:
			xfree(mem);
			break;
		case ALLOC_ARENA:
			break;
		case ALLOC_FIXED_POOL:
			pool_free(al, mem);
			break;
		case ALLOC_MULTI_POOL:
			mp_free(al, mem, sz);
			break;
		default:
			__builtin_unreachable();
	}
}

