#ifndef ALLOC_H
#define ALLOC_H

#include "common.h"


void *xmalloc(len_t s);
void *xcalloc(len_t s, len_t n);
void *xrealloc(void *p, len_t s);
void xfree(void *mem);

#endif /* ALLOC_H */

