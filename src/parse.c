#include <string.h>
#include <assert.h>
#include <stdalign.h>
#include <stdnoreturn.h>
#include <stdlib.h>
#include <stdarg.h>

#include "common.h"
#include "ast.h"
#include "parse.h"
#include "alloc.h"


#define X(w) ident_t kw_ ## w;
KEYWORDS()
#undef X

void init_keywords(ptrmap *m)
{
#define X(w) kw_ ## w = *(ident_t *) pmap_push(m, (void *) fb_set(sizeof (# w) - 1, # w), identifier_hash);
	KEYWORDS()
#undef X
}

enum { N = 16 * 1024 };
struct block {
	mem_temp tmp;
	alignas (16) char mem[];
};

static noreturn void fatal_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	assert(0);
	exit(1);
}

static int is_token(parser *p, token_kind kind)
{
	return p->l.tok.kind == kind;
}

static int match_token(parser *p, token_kind kind)
{
	int i = is_token(p, kind);
	if (i) lexer_next(&p->l);
	return i;
}

static token expect_token(parser *p, token_kind kind)
{
	if (!is_token(p, kind))
		fatal_error("expected token kind %d, got %d\n", kind, p->l.tok.kind);
	token tok = p->l.tok;
	lexer_next(&p->l);
	return tok;
}

static int is_keyword(parser *p, ident_t kw)
{
	return is_token(p, TK_NAME) && p->l.tok.tid == kw;
}

static int match_keyword(parser *p, ident_t kw)
{
	int i = is_keyword(p, kw);
	if (i) lexer_next(&p->l);
	return i;
}

static void expect_keyword(parser *p, ident_t kw)
{
	if (!match_keyword(p, kw)) {
		char kw_str[32] = { 0 };
		len_t kw_len;
		const char *kw_repr = repr_ident(kw, &kw_len);
		memcpy(kw_str, kw_repr, kw_len);
		fatal_error("expected keyword `%s`, got `%s`\n", kw_str, repr_ident(p->l.tok.tid, NULL));
	}
}

void parser_init(parser *p, stream s)
{
	lexer_init(&p->l, &system_allocator, s);
	ast_init(&p->ast);
	// TODO: obviously not ideal
	struct block *blk = gen_alloc(&system_allocator, sizeof *blk + N);
	tmp_init(&blk->tmp, NULL, &blk->mem, N);
	mp_init(&p->mp, &blk->tmp, 16, 4096);
	init_keywords(&p->l.ids);
}

void parser_fini(parser *p)
{
	lexer_fini(&p->l);
	ast_fini(&p->ast);
	mp_fini(&p->mp);
	struct block *blk = (struct block *) (p->mp._packed >> 12);
	gen_free(&system_allocator, blk, sizeof *blk + N);
}

void parse(parser *p)
{
	small_buf ctrs = 0;
	while (1) {
		lexer_next(&p->l);
		if (match_keyword(p, kw_func)) {
			owo_construct ctr = parse_func(p);
			sm_add(&p->mp, &ctrs, &ctr, PTRSZ);
		}
		else {
			assert(is_token(p, TK_EOF));
			p->ast.ctrs = ctrs;
			return;
		}
	}
}

owo_type parse_type(parser *p)
{
	expect_keyword(p, kw_int);
	owo_type t = owo_tint;
	while (match_token(p, TK_AT))
		t = owt_ptr(t);
	return t;
}

owo_expr parse_expr(parser *p)
{
	assert(is_token(p, TK_INT));
	return owe_int(p->l.tok.tint);
}

owo_stmt parse_stmt(parser *p)
{
	expect_keyword(p, kw_return);
	owo_expr rval = parse_expr(p);
	return ows_sreturn(rval);
}

small_buf parse_stmt_block(parser *p)
{
	small_buf body = 0;
	expect_token(p, TK_LBRACE);
	while (!match_token(p, TK_RBRACE)) {
		owo_stmt stmt = parse_stmt(p);
		sm_add(&p->mp, &body, &stmt, PTRSZ);
		lexer_next(&p->l);
	}
	return body;
}

owo_construct parse_func(parser *p)
{
	token tok = expect_token(p, TK_NAME);
	ident_t name = tok.tid;
	expect_token(p, TK_LPAREN);
	small_buf params = 0;
	for (int i = 0; !match_token(p, TK_RPAREN); i++) {
		if (i)
			expect_token(p, TK_COMMA);
		tok = expect_token(p, TK_NAME);
		expect_token(p, TK_COLON);
		struct owo_param param;
		param.type = parse_type(p);
		sm_add(&p->mp, &params, &param, sizeof param);
	}
	expect_token(p, TK_COLON);
	owo_type ret = parse_type(p);
	small_buf body = parse_stmt_block(p);
	return owc_funcdef(&p->mp, name, ret, params, body);
}

