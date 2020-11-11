#ifndef BUFFER_H
#define BUFFER_H

#include "begincpp.h"

#include "common.h"


// useful for strings, where the pointer isn't aligned
typedef uintptr_t fixed_buf;
	// bits [48..64[: len
	// bits [ 0..48[: ptr

typedef uintptr_t small_buf;
	// bit 64: reserved, 0 for now
	// bits [48..64[: len => has to be less or equal to 2**15-1
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
void *sm_fit(allocator al, small_buf *b, len_t n, len_t objsz);
void *sm_reserve(allocator al, small_buf *b, len_t n, len_t objsz);

// MUST BE 1 ARGUMENT ONLY
#define sm_iter(T, buf, it, ...) \
	do { \
		T *_sm_iter_start_ ## it = sm_mem((buf)); \
		T *_sm_iter_end_ ## it   = _sm_iter_start_ ## it + sm_len((buf)); \
		for (T *it ## _address = _sm_iter_start_ ## it; it ## _address != _sm_iter_end_ ## it; it ## _address++) \
			{ \
				T it = *it ## _address; \
				__VA_ARGS__ \
			} \
	} while (0)
	

#include "endcpp.h"

#endif /* BUFFER_H */

