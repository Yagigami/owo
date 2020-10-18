#include "common.h"
#include "alloc.h"

#include <assert.h>
#include <string.h>


len_t sb_cap(stretchy_buf buf)
{
	return 1ll << buf.log2_cap;
}

void *sb_add(stretchy_buf *buf, void *obj, len_t objsz)
{
	len_t len = buf->len + 1;
	if (!buf->mem || len > sb_cap(*buf))
		sb_resize(buf, len * objsz);
	memcpy((char *) buf->mem + buf->len, obj, objsz);
	buf->len = len;
	return (char *) buf->mem + buf->len;
}

void sb_resize(stretchy_buf *buf, len_t n)
{
	len_t cap = sb_cap(*buf) * 2;
	if (cap < n) cap = n;
	buf->mem = xrealloc(buf->mem, n);
}

void sb_fini(stretchy_buf *buf)
{
	xfree(buf->mem);
#ifndef NDEBUG
	memset(buf, 0, sizeof *buf);
#endif
}

void sb_shrink_into(fixed_buf *restrict dst, const stretchy_buf *restrict src, len_t objsz)
{
	fb_set_len(dst, src->len);
	fb_set_mem(dst, xrealloc(src->mem, objsz * src->len));
}

identifier *strdup_range(const char *start, const char *end)
{
	identifier *id = xmalloc(sizeof *id + (end - start));
	id->len = end - start;
	memcpy(id->str, start, id->len);
	return id;
}

identifier *strdup_len(const char *start, len_t len)
{
	return strdup_range(start, start + len);
}

len_t fb_len(fixed_buf b)
{
	static_assert(sizeof (uintptr_t) == 8, "");
	return b >> 48;
}

void *fb_mem(fixed_buf b)
{
	return (void *) SEXTEND(b, 16);
}

void fb_set_len(fixed_buf *b, len_t l)
{
	*b = (*b & BITS(48)) | (l << 48);
}

void fb_set_mem(fixed_buf *b, void *m)
{
	*b = (*b & BITRANGE(48, 64)) | ((uintptr_t) m & BITS(48));
}

fixed_buf fb_set(len_t len, void *mem)
{
	return (len << 48) | ((uintptr_t) mem & BITS(48));
}

len_t sm_len(small_buf b)
{
	return b >> 48;
}

len_t sm_cap(small_buf b)
{
	return 1 << (b & 0xF);
}

void *sm_mem(small_buf b)
{
	return (void *) SEXTEND(b & BITRANGE(4, 48), 16);
}

void sm_set_mem(small_buf *b, void *mem)
{
	assert(((uintptr_t) mem & 0xF) == 0);
	assert(((uintptr_t) mem >> 48) == 0);
	*b = (*b & ~BITRANGE(4, 48)) | (uintptr_t) mem;
}

void *sm_add(small_buf *b, void *obj, len_t objsz)
{
	len_t len = sm_len(*b);
	if (!*b || len + 1 > sm_cap(*b))
		sm_resize(b, objsz);
	void *dst = (char *) sm_mem(*b) + len * objsz;
	memcpy(dst, obj, objsz);
	len++;
	*b = (len << 48) | (*b & BITS(48));
	return dst;
}

void *sm_resize(small_buf *b, len_t objsz)
{
	len_t cap = sm_cap(*b) * 2;
	void *new = xrealloc(sm_mem(*b), cap * objsz);
	assert(cap < (1 << 15));
	++*b;
	sm_set_mem(b, new);
	return new;
}

void *sm_shrink_into(small_buf *restrict dst, small_buf src, len_t objsz)
{
	len_t len = sm_len(src);
	len_t log2_cap = 63 - __builtin_clzll(len);
	void *new = xrealloc(sm_mem(src), (1 << log2_cap) * objsz);
	sm_set_mem(dst, new);
	*dst |= (len << 48);
	*dst |= log2_cap;
	return new;
}

