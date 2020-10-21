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
#define BITRANGE(a, b) (BITS((b)) - BITS((a)))
#define SEXTEND(x, n) ((intmax_t) ((uintmax_t) (x) * BIT((n))) / (1LL << (n)))
#define PTRSZ (sizeof (void *))
#define ISPOW2(x) (((x) & ((x) - 1)) == 0)
#define ALIGN_DOWN_P2(x, y) ((x) & ~((y) - 1))
#define ALIGN_UP_P2(x, y) ALIGN_DOWN_P2((x) + (y) - (1), (y))
#define IS_LOW_PTR(p) ((uintptr_t) p < BIT(48))
#define IS_NICE_PTR8(p) (IS_LOW_PTR((p)) && ((uintptr_t) (p) & 7) == 0)
#define IS_NICE_PTR16(p) (IS_LOW_PTR((p)) && ((uintptr_t) (p) & 15) == 0)

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#define BAD __attribute__((error("bad")))
#define DEPRECATED(reason) __attribute__((deprecated(# reason)))
#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)
#define NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#define REALLOC_LIKE __attribute__((returns_nonnull, assume_aligned(16), alloc_size(2)))
#define MALLOC_LIKE __attribute__((malloc)) REALLOC_LIKE

typedef ptrdiff_t len_t;

typedef uintptr_t ident_t;

typedef struct stream {
	len_t len;
	char *buf;
} stream;

typedef void *restrict allocator;

const char *repr_ident(ident_t id, len_t *len);

#define thread_local _Thread_local

#include "buf.h"

#ifdef  __cplusplus
}
#endif

#endif /* COMMON_H */

