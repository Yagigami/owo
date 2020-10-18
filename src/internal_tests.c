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

