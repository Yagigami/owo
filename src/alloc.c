#include "common.h"
#include "alloc.h"

#include <stdlib.h>
#include <stdio.h>


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


