#include <assert.h>
#include <stdint.h>
#include <string.h>

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
	sm_add(&b, a + 0, sizeof *a);
	sm_add(&b, a + 1, sizeof *a);
	sm_add(&b, a + 2, sizeof *a);
	sm_add(&b, a + 3, sizeof *a);
	sm_add(&b, a + 4, sizeof *a);
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

	arena_init(&ar, N);
	p = arena_alloc(&ar, 8);
	memset(p, 13, 8);
	p = arena_alloc(&ar, 13);
	memset(p, 14, 13);
	p = arena_alloc(&ar, 76);
	memset(p, 89, 76);
	p = arena_alloc(&ar, 15);
	memset(p, 90, 15);
	arena_fini(&ar);
}

static void test_pool(void)
{
	mem_pool p;
	enum { N = 64 };

	pool_init(&p, N, 16);
	void *p1 = pool_alloc(&p);
	memset(p1, 1, 16);
	void *p2 = pool_alloc(&p);
	memset(p2, 2, 16);
	void *p3 = pool_alloc(&p);
	memset(p3, 3, 16);
	pool_free(&p, p2);
	void *p4 = pool_alloc(&p);
	assert(p4 == p2);
	memset(p4, 4, 16);
	pool_free(&p, p1);
	pool_free(&p, p3);
	assert(pool_alloc(&p) == p3);
	assert(pool_alloc(&p) == p1);
	pool_fini(&p);
}

static void test_mpool(void)
{
	multipool mp;
	enum { N = 256 };

	mp_init(&mp, 8, 32, N);
	void *p1 = mp_alloc(&mp, 16);
	memset(p1, 1, 16);
	void *p2 = mp_alloc(&mp, 8);
	memset(p2, 2, 8);
	void *p3 = mp_alloc(&mp, 16);
	memset(p3, 3, 16);
	mp_free(&mp, p1, 16);
	mp_free(&mp, p2, 8);
	void *p4 = mp_alloc(&mp, 32);
	memset(p4, 4, 32);
	void *p5 = mp_alloc(&mp, 16);
	assert(p5 == p1);
	memset(p5, 5, 16);
	void *p6 = mp_alloc(&mp, 8);
	assert(p6 == p2);
	memset(p6, 6, 8);
	mp_fini(&mp);
}

void test_gen_alloc(void)
{
	multipool mp;
	enum { N = 256 };

	mp_init(&mp, 8, 64, N);

	void *p1 = gen_alloc(&mp, 13);
	memset(p1, 1, 16);
	gen_free(&mp, p1, 13);

	mp_fini(&mp);
}

void test_alloc(void)
{
	test_arena();
	test_pool();
	test_mpool();

	test_gen_alloc();
}
