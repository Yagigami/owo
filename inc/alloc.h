#ifndef ALLOC_H
#define ALLOC_H

#include "begincpp.h"

#include "common.h"
#include "buf.h"

#include <setjmp.h>


// TODO: make allocators that take an upstream
// 	allocator *all* have their 1st member
// 	be { alloc_base; upstream<<16 [; log2_sth ] }

__attribute__((malloc, assume_aligned (16), alloc_size (1), returns_nonnull)) void *
	xmalloc(len_t s);
__attribute__((malloc, assume_aligned (16), alloc_size (1, 2), returns_nonnull)) void *
	xcalloc(len_t s, len_t n);
REALLOC_LIKE void *xrealloc(void *p, len_t s);
void xfree(void *mem);

typedef enum alloc_base {
	ALLOC_NONE = 0,
	ALLOC_DEFAULT,
	ALLOC_ARENA,
	ALLOC_FIXED_POOL,
	ALLOC_MULTI_POOL,
	ALLOC_TMP,
	ALLOC_NUM,
} alloc_base;

typedef struct mem_arena {
	// byte #1: base
	// rest : upstream<<8
	uintptr_t _packed;
	char *cur, *start, *end;
} mem_arena;

typedef char *mem_arena_mark;

NONNULL(1, 2) void arena_init(mem_arena *ar, allocator upstream, len_t sz);
NONNULL(1) void arena_fini(mem_arena *ar);
NONNULL(1) MALLOC_LIKE void *arena_alloc(mem_arena *ar, len_t sz);
NONNULL(1) void arena_free(mem_arena *ar, void *mem, len_t sz);
NONNULL(1) MALLOC_LIKE __attribute__((alloc_align(3))) void *
	arena_alloc_align(mem_arena *ar, len_t sz, len_t align);

NONNULL(1) mem_arena_mark arena_mark(mem_arena *ar);
NONNULL(1, 2) void arena_rewind(mem_arena *ar, mem_arena_mark mark);

typedef struct mem_pool {
	// byte #1 : base
	// bits [ 8..64[: ptr<<8
	uintptr_t _packed;
	allocator upstream;
} mem_pool;

NONNULL(1) void pool_init(mem_pool *p, allocator upstream, len_t objsz);
NONNULL(1) MALLOC_LIKE void *pool_alloc(mem_pool *p, len_t sz);
NONNULL(1, 2) void pool_free(mem_pool *restrict p, void *mem);

typedef struct multipool {
	// byte #1: base
	// 4 bits: log2_minsz - 4
	// rest: upstream<<12
	uintptr_t _packed;
	fixed_buf buf;
} multipool;

NONNULL(1) void mp_init(multipool *mp, allocator upstream, len_t minsz, len_t maxsz);
NONNULL(1) void mp_fini(multipool *mp);
NONNULL(1) MALLOC_LIKE void * mp_alloc(multipool *mp, len_t sz);
NONNULL(1, 2) void mp_free(multipool *restrict mp, void *mem, len_t sz);

typedef struct mem_temp {
	// first byte : base
	// rest : end ptr<<8
	uintptr_t _packed;
	char *cur;
	jmp_buf *ctx;
} mem_temp;

NONNULL(1) void tmp_init(mem_temp *tmp, jmp_buf *ctx, void *restrict mem, len_t sz);
NONNULL(1) MALLOC_LIKE void * tmp_alloc(mem_temp *tmp, len_t sz);

extern NONNULL(1) MALLOC_LIKE void *gen_alloc(allocator al, len_t sz);
extern NONNULL(1, 2) void gen_free(allocator al, void *mem, len_t sz);
// however, for now, `mem` may be NULL
extern NONNULL(1) REALLOC_LIKE void *gen_realloc(allocator al, len_t new_sz, void *mem, len_t sz);

extern alloc_base system_allocator;

#include "endcpp.h"

#endif /* ALLOC_H */

