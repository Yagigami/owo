#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>

#include "alloc.h"
#include "common.h"
#include "ptrmap.h"

void test_macros(void)
{
	assert(BIT(6) == 0x40);
	assert(BITS(11) == 0x7FF);
	assert(SEXTEND(0x00400000, 9+32) == (int) 0xFFC00000);

	uint8_t buf[1024];
	intptr_t i = (intptr_t) buf & BITRANGE(0, 48);
	uint8_t *p = (uint8_t *) SEXTEND(i, 16);
	assert(p == buf);
}

typedef struct {
	char s[8];
	int id;
} test_pmap_obj;

static hash_t test_pmap_hash(key_t k)
{
	test_pmap_obj *o = k;
	return strcmp(o->s, "foo");
}

void test_pmap(void)
{
	ptrmap m = {0};

	test_pmap_obj a = { "abc", 0 };
	test_pmap_obj b = { "cedzjqk", 1 };
	test_pmap_obj c = { "adzf", 2 };
	test_pmap_obj d = { "ad8r", 3 };

	test_pmap_obj **pa = (test_pmap_obj **) pmap_push(&m, &a, test_pmap_hash);
	test_pmap_obj **pb = (test_pmap_obj **) pmap_push(&m, &b, test_pmap_hash);
	test_pmap_obj **pc = (test_pmap_obj **) pmap_push(&m, &c, test_pmap_hash);
	test_pmap_obj **pd = (test_pmap_obj **) pmap_push(&m, &d, test_pmap_hash);

	assert(pa[0]->id == 0);
	assert(pb[0]->id == 1);
	assert(pc[0]->id == 2);
	assert(pd[0]->id == 3);

	pmap_fini(&m);
}

void test_buf(void)
{
	char *s = "abcdef";
	fixed_buf b;
	fb_set_mem(&b, s);
	assert(fb_mem(b) == s);
	fb_set_len(&b, 6);
	assert(fb_mem(b) == s);
	assert(fb_len(b) == 6);
}

void test_sm(void)
{
	int a[] = { 1, 2, 3, 4, 5, }, n = sizeof a / sizeof *a;
	small_buf b = 0;
	sm_add(&system_allocator, &b, a + 0, sizeof *a);
	sm_add(&system_allocator, &b, a + 1, sizeof *a);
	sm_add(&system_allocator, &b, a + 2, sizeof *a);
	sm_add(&system_allocator, &b, a + 3, sizeof *a);
	sm_add(&system_allocator, &b, a + 4, sizeof *a);
	int *p = sm_mem(b);
	assert(p[0] == 1);
	assert(p[1] == 2);
	assert(p[2] == 3);
	assert(p[3] == 4);
	assert(p[4] == 5);
	assert(sm_len(b) == n);
	assert(sm_cap(b) == 8);
	xfree(p);
}

static void test_arena(void)
{
	mem_arena ar;
	void *p;
	enum { N = 64 };

	arena_init(&ar, &system_allocator, N);
	p = arena_alloc(&ar, ALIGN_UP_P2(8, 16));
	memset(p, 13, 8);
	p = arena_alloc(&ar, ALIGN_UP_P2(13, 16));
	memset(p, 14, 13);
	p = arena_alloc(&ar, ALIGN_UP_P2(76, 16));
	memset(p, 89, 76);
	p = arena_alloc(&ar, ALIGN_UP_P2(15, 16));
	memset(p, 90, 15);
	arena_fini(&ar);
}

static void test_pool(void)
{
	mem_temp tmp;
	mem_pool p;
	enum { N = 64 };

	char a[N];
	tmp_init(&tmp, NULL, a, N);
	pool_init(&p, &tmp, 16);
	void *p1 = pool_alloc(&p, 16);
	memset(p1, 1, 16);
	void *p2 = pool_alloc(&p, 16);
	memset(p2, 2, 16);
	void *p3 = pool_alloc(&p, 16);
	memset(p3, 3, 16);
	pool_free(&p, p2);
	void *p4 = pool_alloc(&p, 16);
	assert(p4 == p2);
	memset(p4, 4, 16);
	pool_free(&p, p1);
	pool_free(&p, p3);
	assert(pool_alloc(&p, 16) == p3);
	assert(pool_alloc(&p, 16) == p1);
}

static void test_mpool(void)
{
	mem_temp tmp;
	multipool mp;
	enum { N = 256 };
	char a[N];

	tmp_init(&tmp, NULL, a, N);
	mp_init(&mp, &tmp, 16, 32);
	void *p1 = mp_alloc(&mp, 16);
	memset(p1, 1, 16);
	void *p2 = mp_alloc(&mp, 16);
	memset(p2, 2, 16);
	void *p3 = mp_alloc(&mp, 16);
	memset(p3, 3, 16);
	mp_free(&mp, p1, 16);
	mp_free(&mp, p2, 16);
	void *p4 = mp_alloc(&mp, 32);
	memset(p4, 4, 32);
	void *p5 = mp_alloc(&mp, 16);
	assert(p5 == p2);
	memset(p5, 5, 16);
	void *p6 = mp_alloc(&mp, 16);
	assert(p6 == p1);
	memset(p6, 6, 16);
	mp_fini(&mp);
}

void test_gen_alloc(void)
{
	mem_temp tmp;
	multipool mp;
	enum { N = 256 };
	char a[N];

	tmp_init(&tmp, NULL, a, N);
	mp_init(&mp, &tmp, 16, 64);

	void *p1 = gen_alloc(&mp, ALIGN_UP_P2(13, 16));
	memset(p1, 1, 13);
	gen_free(&mp, p1, ALIGN_UP_P2(13, 16));

	mp_fini(&mp);
}

void test_tmp(void)
{
	mem_temp tmp;
	enum { N = 1024 * 4 };
	char a[N];
	jmp_buf ctx;
	if (setjmp(ctx) == 0) {
		tmp_init(&tmp, &ctx, a, N);

		void *p;
		p = tmp_alloc(&tmp, 16);
		memset(p, 1, 16);
		p = tmp_alloc(&tmp, 16);
		memset(p, 2, 16);
		p = tmp_alloc(&tmp, 16);
		memset(p, 3, 16);
	} else {
		fprintf(stderr, "%d bytes is not enough memory!\n", N);
	}
}

void test_alloc(void)
{
	test_arena();
	test_pool();
	test_mpool();
	test_tmp();

	test_gen_alloc();
}
