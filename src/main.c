#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "common.h"
#include "token.h"

void test_macros(void)
{
	assert(BIT(6) == 0x40);
	assert(BITS(11) == 0x7FF);
	assert(SEXTEND(0x00400000, 9+32) == (int) 0xFFC00000);
}

void test_token(void)
{
	lexer l;
	stream s;
	s.buf = 
		"func main(argc: int, argv: string@): int\n"
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
}

void run_tests(void)
{
	test_macros();
	test_token();
}

int main(int argc, char **argv)
{
	(void) argv[argc];

	run_tests();

	return 0;
}

