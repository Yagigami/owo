#include "common.h"
#include "alloc.h"
#include "buf.h"

#include <assert.h>
#include <string.h>


void *sm_add(allocator al, small_buf *b, obj_t obj, len_t objsz)
{
	len_t len = sm_len(*b);
	if (!*b || len + 1 > sm_cap(*b))
		sm_resize(al, b, objsz);
	void *dst = (char *) sm_mem(*b) + len * objsz;
	memcpy(dst, obj, objsz);
	assert(len < (1 << 15));
	sm_add_len(b, 1);
	return dst;
}

void *sm_resize(allocator al, small_buf *b, len_t objsz)
{
	len_t old_cap = sm_cap(*b);
	len_t cap = old_cap * 2;
	void *new = gen_realloc(al, cap * objsz, sm_mem(*b), old_cap * objsz);
	assert(cap < (1 << 15));
	sm_add_cap(b, 1);
	sm_set_mem(b, new);
	return new;
}

void *sm_shrink_into(allocator al, small_buf *restrict dst, small_buf src, len_t objsz)
{
	len_t len = sm_len(src);
	if (len == 0) {
		*dst = 0;
		return NULL;
	}
	len_t log2_cap = 63 - __builtin_clzll(len);
	if (len * objsz < 16) {
		*dst = src;
		return sm_mem(src);
	}
	void *new = gen_realloc(al, (1 << log2_cap) * objsz, sm_mem(src), sm_cap(src) * objsz);
	assert(IS_NICE_PTR16(new));
	*dst = sm_set(len, log2_cap, new);
	return new;
}

void *sm_fit(allocator al, small_buf *b, len_t n, len_t objsz)
{
	assert(n > 0);
	small_buf old = *b;
#ifndef NDEBUG
	len_t shamt = n != 1 ? 64 - __builtin_clzll(n - 1): 0;
#else
	static_assert(64 - __builtin_clzll(0) == 0, ""); // UNDEFINED BEHAVIOR
	len_t shamt = 64 - __builtin_clzll(n - 1);
#endif
	len_t old_cap = sm_cap(old);
	len_t len     = sm_len(old);
	len_t cap     = 1LL << shamt;
	assert(cap < (1LL << 15));
	char *new = gen_realloc(al, cap * objsz, sm_mem(old), old_cap * objsz);
	*b = sm_set(len, shamt, new);
	return new + len * objsz;
}

void *sm_reserve(allocator al, small_buf *b, len_t n, len_t objsz)
{
	return sm_fit(al, b, sm_len(*b) + n, objsz);
}

void *vec_add(allocator al, vector *v, obj_t obj, len_t objsz)
{
	len_t n = v->len + 1;
	if (n > v->cap)
		vec_fit(al, v, v->cap ? 2 * v->cap: 1, objsz);
	void *dst = (char *) v->mem + v->len;
	memcpy(dst, obj, objsz);
	v->len = n;
	return dst;
}

void *vec_fit(allocator al, vector *v, len_t n, len_t objsz)
{
	// assert(n > v->cap);
	if (n <= v->cap) return v->mem;
	char *new = gen_realloc(al, n * objsz, v->mem, v->cap * objsz);
	v->cap = n;
	return v->mem = new;
}

void *vec_reserve(allocator al, vector *v, len_t n, len_t objsz)
{
	return vec_fit(al, v, v->len + n, objsz);
}

void vec_extend(allocator al, vector *v, const void *restrict src, len_t n, len_t objsz)
{
	char *restrict dst = vec_reserve(al, v, n, objsz);
	memcpy(dst + v->len * objsz, src, n * objsz);
	v->len += n;
}

