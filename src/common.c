#include "common.h"
#include "alloc.h"
#include "buf.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>


const char *repr_ident(ident_t id, len_t *len)
{
	enum { N = 64 };
	static char buf[N];
	int res = snprintf(buf, N, "%.*s", (int) fb_len(id), (char *) fb_mem(id));
	if (len) *len = res;
	return buf;
}
