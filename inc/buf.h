#ifndef BUFFER_H
#define BUFFER_H

#include "begincpp.h"

#include "common.h"

#include <assert.h>

static_assert(sizeof (uintptr_t) == 8, "");

typedef uintptr_t fixed_buf;
	// bits [ 0..20[ = len
	// bits [20..64[ = ptr<<4

// may want to try another layout (doesn't require ugly 8-byte immediates)
//   bits [20..64[: mem
//   bits [16..20[: log2_cap
//   bits [ 0..16[: len
//   (may reserve some pointer bits, the first 3 are always 0 and the 4th is very likely to be too)
typedef uintptr_t small_buf;
	// bit 64: reserved, 0 for now
	// bits [48..64[: len => has to be less or equal to 2**15-1
	// bits [ 4..48[: mem
	// bits [ 0.. 4[: log2_cap

typedef struct vector {
	void *mem;
	int len, cap;
} vector;

static inline len_t fb_len(fixed_buf b)              { return b & BITS(20); }
static inline void *fb_mem(fixed_buf b)              { return (void *) ((b >> 20) << 4); }
static inline fixed_buf fb_set(len_t len, void *mem) { return len | ((uintptr_t) mem << 16); }

fixed_buf fb_shrink(allocator al, vector v, len_t objsz);

typedef const void *restrict obj_t;

#define BUF_H_DEPREC_SMALL_BUF DEPRECATED("cause too much bugs, and `vector` / `fixed_buf` are always better") 

BUF_H_DEPREC_SMALL_BUF static inline len_t sm_len(small_buf b) { return b >> 48;                        }
BUF_H_DEPREC_SMALL_BUF static inline len_t sm_cap(small_buf b) { return 1LL << (b & 0xF);               }
BUF_H_DEPREC_SMALL_BUF static inline void *sm_mem(small_buf b) { return (void *) (b & BITRANGE(4, 48)); }

BUF_H_DEPREC_SMALL_BUF static inline void sm_add_cap(small_buf *b, len_t log2_cap)
{
	*b += log2_cap;
}

BUF_H_DEPREC_SMALL_BUF static inline small_buf sm_set(len_t len, len_t log2_cap, const void *mem)
{
	return (len << 48) | log2_cap | (uintptr_t) mem;
}

BUF_H_DEPREC_SMALL_BUF static inline void sm_set_mem(small_buf *b, void *mem)
{
	assert(IS_NICE_PTR16(mem));
	*b = (*b & ~BITRANGE(4, 48)) | (uintptr_t) mem;
}

BUF_H_DEPREC_SMALL_BUF static inline void sm_add_len(small_buf *b, len_t n)
{
	assert(n < (1LL << 15) - 1);
	*b += n << 48;
}

BUF_H_DEPREC_SMALL_BUF void *sm_add(allocator al, small_buf *b, obj_t obj, len_t objsz);
BUF_H_DEPREC_SMALL_BUF void *sm_resize(allocator al, small_buf *b, len_t objsz);
BUF_H_DEPREC_SMALL_BUF void *sm_shrink_into(allocator al, small_buf *restrict dst, small_buf src, len_t objsz);
BUF_H_DEPREC_SMALL_BUF void *sm_fit(allocator al, small_buf *b, len_t n, len_t objsz);
BUF_H_DEPREC_SMALL_BUF void *sm_reserve(allocator al, small_buf *b, len_t n, len_t objsz);

// example use ```c
// 	sm_iter(float, my_buf, x, {
// 		float y = f(x);
// 		printf("f(%f) = %f\n", x, y);
// 	});
// ```
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
	

// TODO: make this kind of generic function let the user do the copy themselves
void *vec_add(allocator al, vector *v, obj_t obj, len_t objsz);
void *vec_fit(allocator al, vector *v, len_t n, len_t objsz);
void *vec_reserve(allocator al, vector *v, len_t n, len_t objsz);
void vec_extend(allocator al, vector *v, const void *restrict src, len_t n, len_t objsz);

#include "endcpp.h"

#endif /* BUFFER_H */

