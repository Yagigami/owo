#ifndef ALLOC_H
#define ALLOC_H

#include "common.h"
#include "buf.h"


void *xmalloc(len_t s);
void *xcalloc(len_t s, len_t n);
void *xrealloc(void *p, len_t s);
void xfree(void *mem);

typedef enum alloc_base {
	ALLOC_DEFAULT = 0,
	ALLOC_ARENA,
	ALLOC_FIXED_POOL,
	ALLOC_MULTI_POOL,
	ALLOC_NUM,
} alloc_base;

typedef struct mem_arena {
	// bits [56..64[: base
	// bits [ 0..56[: start
	uintptr_t _packed;
	char *cur, *end;
} mem_arena;

void arena_init(mem_arena *ar, len_t sz);
void arena_fini(mem_arena *ar);
void *arena_alloc(mem_arena *ar, len_t sz);
void *arena_alloc_align(mem_arena *ar, len_t sz, len_t align);

typedef struct mem_pool {
	// bits [56..64[: base
	// bits [48..56[: reserved
	// bits [ 3..48[: ptr
	// bits [ 0.. 3[: log2_objsz
	uintptr_t _packed;
	mem_arena ar;
} mem_pool;

void pool_init(mem_pool *p, len_t sz, len_t objsz);
void pool_fini(mem_pool *p);
void *pool_alloc(mem_pool *p);
void pool_free(mem_pool *p, void *mem);

typedef struct multipool {
	int8_t base;
	small_buf buf;
} multipool;

void mp_init(multipool *mp, len_t minsz, len_t maxsz, len_t blksz);
void mp_fini(multipool *mp);
void *mp_alloc(multipool *mp, len_t sz);
void mp_free(multipool *mp, void *mem, len_t sz);

extern void *gen_alloc(allocator al, len_t sz);
extern void gen_free(allocator al, void *mem, len_t sz);

extern alloc_base system_allocator;

#endif /* ALLOC_H */

