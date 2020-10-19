#include "common.h"
#include "alloc.h"
#include "buf.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>


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

const char *repr_ident(ident_t id, len_t *len)
{
	enum { N = 64 };
	static char buf[N];
	int res = snprintf(buf, N, "%.*s", (int) fb_len(id), (char *) fb_mem(id));
	if (len) *len = res;
	return buf;
}
