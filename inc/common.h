#ifndef COMMON_H
#define COMMON_H

#define BIT(n) (1ULL << (n))
#define BITS(n) (BIT((n) % (sizeof (uint64_t) * CHAR_BIT)) - 1)
#define BITRANGE(a, b) (BITS((b) - 1) - BITS((a)))
#define SEXTEND(x, n) ((intmax_t) ((uintmax_t) (x) * BIT((n))) / (1LL << (n)))

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

#endif /* COMMON_H */

