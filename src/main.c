#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "alloc.h"
#include "common.h"
#include "token.h"
#include "ptrmap.h"
#include "parse.h"
#include "internal_tests.h"
#include "bytecode.h"
#include "x86-64.h"
#include "elf-rename-me.h"

void test_token(void)
{
	printf("==== LEXER ====\n");
	stream s;
	lexer l;
	s.buf = 
		"func main(argc: int, argv: string): int\n"
		"{\n"
		"	return 0;\n"
		"}\n"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		;
	s.len = strlen(s.buf);
	s.buf = memcpy(xmalloc(s.len + 16), s.buf, s.len + 16);
	lexer_init(&l, s, lex_str_default, NULL);

	do {
		lexer_next(&l);
		token_print(stdout, l.tok);
	} while (l.tok.kind != TK_EOF);

	lexer_fini(&l);
	xfree(s.buf);
	printf("\n\n");
}

void test_parser(void)
{
	printf("==== PARSER ====\n");
	parser p;
	stream s;
	s.buf =
		"func main(a: int@, b: int@@@@@@): int\n"
		"{\n"
		"	var x: int = 13;\n"
		"	return 0;\n"
		"}\n"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		;
	s.len = strlen(s.buf);
	s.buf = memcpy(xmalloc(s.len + 16), s.buf, s.len + 16);
	parser_init(&p, s, lex_str_default);
	parse(&p);
	parser_fini(&p);
	xfree(s.buf);
	printf("\n\n");
}

void test_bytecode(void)
{
	parser p;
	stream s;
	bc_unit u;
	gen_x64 g;
	printf("==== BYTECODE ====\n");
	s.buf = 
		"func main(): int"
		"{\n"
		"	return 0;\n"
		"}\n"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		;
	s.len = strlen(s.buf);
	s.buf = memcpy(xmalloc(s.len + 16), s.buf, s.len + 16);
	parser_init(&p, s, lex_str_default);
	parse(&p);

	bcu_init(&u, &system_allocator);
	bct_ast(&u, &p.ast);
	bcu_dump(stdout, &u);
	gx64_init(&g, &system_allocator);
	gx64t_bc(&g, &u);
	gx64_dump(stdout, &g);

	stream f = elf_serialize_x64(&system_allocator, &g, "foo");
	FILE *fp = fopen("foo", "wb");
	fwrite(f.buf, f.len, 1, fp);
	fclose(fp);

	xfree(f.buf);	
	gx64_fini(&g);
	bcu_fini(&u);
	parser_fini(&p);
	xfree(s.buf);

	printf("\n\n");
}

void run_tests(void)
{
	test_macros();
	test_buf();
	test_sm();
	// test_pmap();
	test_alloc();
	test_token();
	test_parser();
	test_bytecode();
}

#if 0
void *gen_alloc(allocator al, len_t sz)
{
	(void) al;
	return xmalloc(sz);
}

void gen_free(allocator al, void *mem, len_t sz)
{
	(void) al, (void) sz;
	xfree(mem);
}
#endif

int main(int argc, char **argv)
{
	(void) argv[argc];

	run_tests();

	return 0;
}

