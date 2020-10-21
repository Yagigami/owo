#ifndef BUFFER_H
#define BUFFER_H

#include "common.h"


// useful for strings, where the pointer isn't aligned
typedef uintptr_t fixed_buf;
	// bits [48..64[: len
	// bits [ 0..48[: ptr

typedef uintptr_t small_buf;
	// bit 64: reserved, 0 for now
	// bits [48..64[: len
	// bits [ 4..48[: mem
	// bits [ 0.. 4[: log2_cap

len_t fb_len(fixed_buf b);
void *fb_mem(fixed_buf b);
void fb_set_len(fixed_buf *b, len_t l);
void fb_set_mem(fixed_buf *b, void *m);
fixed_buf fb_set(len_t len, void *mem);

typedef const void *restrict obj_t;

len_t sm_len(small_buf b);
len_t sm_cap(small_buf b);
void *sm_mem(small_buf b);
void sm_set_mem(small_buf *b, void *mem);
void *sm_add(allocator al, small_buf *b, obj_t obj, len_t objsz);
void *sm_resize(allocator al, small_buf *b, len_t objsz);
void *sm_shrink_into(allocator al, small_buf *restrict dst, small_buf src, len_t objsz);


#endif /* BUFFER_H */

