#include "common.h"
#include "alloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdalign.h>
#include <assert.h>
#include <string.h>
#include <setjmp.h>


#define ARENA_META 16
#define MULTIPOOL_BIAS 4

alloc_base system_allocator = ALLOC_DEFAULT;

void *xmalloc(len_t s)
{
	void *mem = malloc(s);
	if (LIKELY(mem != NULL)) return mem;
	fprintf(stderr, "xmalloc(%zd) failed.\n", s);
	exit(1);
}

void *xrealloc(void *p, len_t s)
{
	void *mem = realloc(p, s);
	if (LIKELY(mem != NULL)) return mem;
	fprintf(stderr, "xrealloc(%p, %zd) failed.\n", p, s);
	exit(1);
}

void *xcalloc(len_t s, len_t n)
{
	void *mem = calloc(s, n);
	if (LIKELY(mem != NULL)) return mem;
	fprintf(stderr, "xcalloc(%zd, %zd) failed.\n", s, n);
	exit(1);
}

void xfree(void *mem)
{
	free(mem);
}

static void arena_meta(void *restrict start, void *restrict next, uint8_t growths)
{
	// ew
	*(void **) start = next;
	((uint8_t *) start)[9] = growths;
}

void arena_init(mem_arena *ar, allocator upstream, len_t sz)
{
	assert(sz > 16);
	ar->upstream = upstream;
	char *start = gen_alloc(ar->upstream, sz);
	// char *start = xmalloc(sz);
	// the first 8 bytes are a `next` pointer
	// the next 8 are for more metadata
	// meta:
	// 	byte #1 counts arena growths
	ar->cur = start + ARENA_META;
	arena_meta(start, NULL, 0);
	ar->end = start + sz;
	ar->_packed = ALLOC_ARENA | ((uintptr_t) start << 8);
	// ar->_packed = ((uintptr_t) ALLOC_ARENA << 56) | (intptr_t) start;
}

void arena_fini(mem_arena *ar)
{
	void *cur = (void *) (ar->_packed >> 8);
	len_t len = (char *) ar->end - (char *) cur;
	do {
		void *next = *(void **) cur;
		gen_free(ar->upstream, cur, len);
		cur = next;
		len /= 2;
		assert(ISPOW2(len) && len > 16);
	} while (cur);
}

void *arena_alloc(mem_arena *ar, len_t sz)
{
	return arena_alloc_align(ar, sz, 16);
}

void *arena_alloc_align(mem_arena *ar, len_t sz, len_t align)
{
	assert(ISPOW2(align));
	char *new = (char *) ALIGN_UP_P2((intptr_t) ar->cur, align);
	if (UNLIKELY(new + sz >= ar->end)) {
		char *start = (void *) (ar->_packed >> 8);
		len_t blk_sz = 2 * (ar->end - start);
		assert(blk_sz > (len_t) ARENA_META + sz + align);
		void *blk = gen_alloc(ar->upstream, blk_sz);
		arena_meta(blk, start, ((uint8_t *) start)[9]);
		start = blk;
		ar->end = start + blk_sz;
		ar->cur = start + ARENA_META;
		// assumes little-endian
		ar->_packed = ALLOC_ARENA | ((uintptr_t) start << 8);
		new = (char *) ALIGN_UP_P2((intptr_t) ar->cur, align);
	}
	ar->cur = new + sz;
	return new;
}

mem_arena_mark arena_mark(mem_arena *ar)
{
	return ar->cur;
}

void arena_rewind(mem_arena *ar, mem_arena_mark mark)
{
	assert(mark >= (char *) (ar->_packed >> 8) && mark < ar->end);
	ar->cur = mark;
}

static void *pool_ptr(const mem_pool *p)
{
	return (void *) (p->_packed >> (8 + MULTIPOOL_BIAS));
}

static void pool_set_ptr(mem_pool *p, void *ptr)
{
	assert(IS_LOW_PTR(ptr));
	len_t len = 8 + MULTIPOOL_BIAS;
	p->_packed = ((uintptr_t) ptr << len) | (p->_packed & BITS(len));
	// assert(((uintptr_t) ptr & 0x7) == 0);
	// intptr_t mask = BITRANGE(3, 48);
	// p->_packed = (p->_packed & ~mask) | (intptr_t) ptr;
}

void pool_init(mem_pool *p, allocator upstream, len_t objsz)
{
	assert(objsz >= 8);
	// arena_init(&p->ar, upstream, sz);
	p->upstream = upstream;
	pool_set_ptr(p, NULL);
	// p->_packed = ((intptr_t) ALLOC_FIXED_POOL << 56) | (63 - __builtin_clzll(objsz));
}

void *pool_alloc(mem_pool *p, len_t sz)
{
	void *mem = pool_ptr(p);
	if (mem) {
		pool_set_ptr(p, *(void **) mem);
		return mem;
	}
	return gen_alloc(p->upstream, sz);
}

void pool_free(mem_pool *restrict p, void *mem)
{
	// does *not* give back to the upstream allocator
	assert(mem);
	*(void **) mem = pool_ptr(p);
	pool_set_ptr(p, mem);
}

void mp_init(multipool *mp, allocator upstream, len_t minsz, len_t maxsz)
{
	assert(minsz > 8);
	len_t len = __builtin_ctzll(maxsz / minsz) + 1;
	len_t biased_log2_minsz = __builtin_ctzll(minsz) - MULTIPOOL_BIAS;
	mem_pool *pools = gen_alloc(upstream, len * sizeof *pools);
	for (len_t sz = minsz, i = 0; sz <= maxsz; sz *= 2, i++) {
		pool_init(pools + i, upstream, sz);
	}
	mp->_packed = ALLOC_MULTI_POOL
		| (biased_log2_minsz << 8)
		| ((uintptr_t) upstream << (8 + MULTIPOOL_BIAS));
	// mp->_packed = ((uintptr_t) ALLOC_MULTI_POOL << 56) | (uintptr_t) upstream;
	mp->buf = fb_set(len, pools);
}

void mp_fini(multipool *mp)
{
	mem_pool *pools = fb_mem(mp->buf);
	len_t len = fb_len(mp->buf);
	allocator upstream = (allocator) (mp->_packed >> (8 + MULTIPOOL_BIAS));
	gen_free(upstream, pools, len * sizeof *pools);
}

static len_t mp_index(const multipool *mp, len_t sz)
{
	len_t l2 = 63 - __builtin_clzll(sz);
	len_t biased_log2_minsz = (mp->_packed >> 8) & BITS(MULTIPOOL_BIAS);
	return l2 - biased_log2_minsz - MULTIPOOL_BIAS;
}

void *mp_alloc(multipool *mp, len_t sz)
{
	mem_pool *pools = fb_mem(mp->buf);
	return pool_alloc(pools + mp_index(mp, sz), sz);
}

void mp_free(multipool *restrict mp, void *mem, len_t sz)
{
	mem_pool *pools = fb_mem(mp->buf);
	pool_free(pools + mp_index(mp, sz), mem);
}

void tmp_init(mem_temp *tmp, jmp_buf *ctx, void *restrict mem, len_t sz)
{
	tmp->ctx = ctx;
	tmp->cur = mem;
	tmp->_packed = ALLOC_TMP | ((uintptr_t) ((char *) mem + sz) << 8);
	// tmp->_packed = ((uintptr_t) ALLOC_TMP << 56) | (uintptr_t) ((char *) mem + sz);
}

void *tmp_alloc(mem_temp *tmp, len_t sz)
{
	char *end = (char *) (tmp->_packed >> 8);
	char *mem = tmp->cur;
	sz = ALIGN_UP_P2(sz, 16);
	if (UNLIKELY(tmp->cur >= end)) {
		if (tmp->ctx)
			longjmp(*tmp->ctx, 1);
		else
			__builtin_unreachable();
	}
	tmp->cur += sz;
	return mem;
}

// can be overridden if you want more allocators
__attribute__((weak))
void *gen_alloc(allocator al, len_t sz)
{
	alloc_base base = *(int8_t *) al;
	switch (base) {
	case ALLOC_DEFAULT   : return xmalloc(sz);
	case ALLOC_ARENA     : return arena_alloc(al, sz);
	case ALLOC_FIXED_POOL: return pool_alloc(al, sz);
	case ALLOC_MULTI_POOL: return mp_alloc(al, sz);
	case ALLOC_TMP       : return tmp_alloc(al, sz);
	case ALLOC_NONE:
	default:
		__builtin_unreachable();
	}
}
__attribute__((weak))
void gen_free(allocator al, void *mem, len_t sz)
{
	alloc_base base = *(int8_t *) al;
#ifndef NDEBUG
	memset(mem, 0x69, sz);
#endif
	switch (base) {
		case ALLOC_DEFAULT:
			xfree(mem);
			break;
		case ALLOC_ARENA:
		case ALLOC_TMP:
			// TODO: may want to log sth on debug
			break;
		case ALLOC_FIXED_POOL:
			pool_free(al, mem);
			break;
		case ALLOC_MULTI_POOL:
			mp_free(al, mem, sz);
			break;
		case ALLOC_NONE:
		default:
			__builtin_unreachable();
	}
}

// none of the current allocators possess any particularly efficient way of reallocating
__attribute__((weak))
void *gen_realloc(allocator al, len_t new_sz, void *mem, len_t sz)
{
	// if (sz == new_sz) return mem;
	void *restrict new = gen_alloc(al, new_sz);
	if (mem) {
		memcpy(new, mem, sz > new_sz ? new_sz: sz);
		gen_free(al, mem, sz);
	}
	return new;
}

