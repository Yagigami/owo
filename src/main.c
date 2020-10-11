#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "alloc.h"
#include "common.h"
#include "token.h"
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

void test_token(void)
{
	lexer l;
	stream s;
	s.buf = 
		"func main(argc: int, argv: string): int\n"
		"{\n"
		"	return = 0;\n"
		"}\n"
		;
	s.len = strlen(s.buf);
	lexer_init(&l, s);

	do {
		lexer_next(&l);
		token_print(stdout, l.tok);
	} while (l.tok.kind != TK_EOF);

	lexer_fini(&l);
}

void run_tests(void)
{
	test_macros();
	test_pmap();
	test_token();
}

int main(int argc, char **argv)
{
	(void) argv[argc];

	run_tests();

	return 0;
}

