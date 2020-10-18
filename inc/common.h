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

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

typedef ptrdiff_t len_t;

typedef struct identifier {
	int8_t len;
	char str[];
} identifier;

typedef const identifier *ident_t;

typedef struct stream {
	len_t len;
	char *buf;
} stream;

#ifdef  __cplusplus
}
#endif

#endif /* COMMON_H */

