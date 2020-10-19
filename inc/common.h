#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
  #ifdef __GNUC__
    #define restrict __restrict
  #else
    #define restrict
  #endif /* GNUC */
  extern "C" {
#endif /* __cplusplus */

#define BIT(n) (1ULL << (n))
#define BITS(n) (BIT((n) % (sizeof (uint64_t) * CHAR_BIT)) - 1)
#define BITRANGE(a, b) (BITS((b) - 1) - BITS((a)))
#define SEXTEND(x, n) ((intmax_t) ((uintmax_t) (x) * BIT((n))) / (1LL << (n)))
#define PTRSZ (sizeof (void *))
#define ISPOW2(x) (((x) & ((x) - 1)) == 0)
#define ALIGN_DOWN_P2(x, y) ((x) & ~((y) - 1))
#define ALIGN_UP_P2(x, y) ALIGN_DOWN_P2((x) + (y) - (1), (y))

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

typedef ptrdiff_t len_t;

typedef uintptr_t ident_t;

// TODO: use small_buf of char instead
typedef struct identifier {
	int8_t len;
	char str[];
} identifier;

// typedef const identifier *ident_t;

typedef struct stream {
	len_t len;
	char *buf;
} stream;

// TODO: replace all those with small_buf
typedef struct stretchy_buf {
	len_t len: sizeof (len_t) * CHAR_BIT - 8;
	len_t log2_cap: 8;
	void *mem;
} stretchy_buf;

len_t sb_cap(stretchy_buf buf);
void *sb_add(stretchy_buf *buf, void *obj, len_t objsz);
void sb_resize(stretchy_buf *buf, len_t n);
void sb_fini(stretchy_buf *buf);
// void sb_shrink_into(fixed_buf *restrict dst, const stretchy_buf *restrict src, len_t objsz);

typedef void *allocator;

#include "buf.h"

#ifdef  __cplusplus
}
#endif

#endif /* COMMON_H */

