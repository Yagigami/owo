#include <string.h>
#include <assert.h>
#include <stdalign.h>
#include <stdlib.h>
#include <stdarg.h>

#include "common.h"
#include "ast.h"
#include "parse.h"
#include "alloc.h"


#define X(w) ident_t kw_ ## w = { .buf = # w };
KEYWORDS()
#undef X

enum { N = 16 * 1024 };
struct block {
	mem_temp tmp;
	alignas (16) char mem[];
};

// TODO: maybe make those functions take a lexer...
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
		fatal_error(ERR_SYNTAX, "expected token kind %d, got %d", kind, p->l.tok.kind);
	token tok = p->l.tok;
	lexer_next(&p->l);
	return tok;
}

static token consume_token(parser *p)
{
	token tok = p->l.tok;
	lexer_next(&p->l);
	return tok;
}

static int is_keyword(parser *p, ident_t kw)
{
	return is_token(p, TK_NAME) && ident_cmp(p->l.tok.tid, kw) == 0;
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
		fatal_error(ERR_SYNTAX, "expected keyword `%.*s`, got `%.*s`", (int) ident_len(&kw), kw.buf, (int) ident_len(&p->l.tok.tid), p->l.tok.tid.buf);
	}
}

void parser_init(parser *p, stream s, lex_str_func *on_str)
{
	lexer_init(&p->l, s, on_str, NULL);
	ast_init(&p->ast);
	// TODO: obviously not ideal
	struct block *blk = gen_alloc(&system_allocator, sizeof *blk + N);
	tmp_init(&blk->tmp, NULL, &blk->mem, N);
	mp_init(&p->mp, &blk->tmp, 16, 4096);
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
	vector decls = {0};
	lexer_next(&p->l);
	while (1) {
		if (match_keyword(p, kw_func)) {
			owo_decl decl = parse_func(p);
			vec_add(&p->mp, &decls, &decl, sizeof decl);
		}
		else {
			assert(is_token(p, TK_EOF));
			p->ast.decls = decls;
			return;
		}
	}
}

owo_type parse_type(parser *p)
{
	expect_keyword(p, kw_int);
	owo_type t = owo_tint;
	while (match_token(p, TK_AT))
		t = type_ptr(t);
	return t;
}

owo_expr parse_expr(parser *p)
{
	token tok = consume_token(p);
	switch (tok.kind) {
	case TK_INT:
		return expr_int(tok.tint);
	default:
		fatal_error(ERR_SYNTAX, "unexpected token %d in expression", tok.kind);
	}
}

owo_stmt parse_stmt(parser *p)
{
	owo_stmt stmt;
	if (match_keyword(p, kw_var)) {
		ident_t name = expect_token(p, TK_NAME).tid;
		expect_token(p, TK_COLON);
		owo_type type = parse_type(p);
		expect_token(p, TK_EQ);
		owo_expr init = parse_expr(p);
		owo_decl decl = decl_var(name, type, init);
		stmt = stmt_decl(decl);
	} else if (match_keyword(p, kw_return)) {
		owo_expr rval = parse_expr(p);
		stmt = stmt_sreturn(rval);
	} else __builtin_unreachable();
	expect_token(p, TK_SCOLON);
	return stmt;
}

fixed_buf parse_stmt_block(parser *p)
{
	vector body = {0};
	expect_token(p, TK_LBRACE);
	while (!match_token(p, TK_RBRACE)) {
		owo_stmt stmt = parse_stmt(p);
		vec_add(&p->mp, &body, &stmt, sizeof stmt);
	}
	return fb_shrink(&p->mp, body, sizeof (owo_stmt));
}

owo_decl parse_func(parser *p)
{
	token tok = expect_token(p, TK_NAME);
	ident_t name = tok.tid;
	expect_token(p, TK_LPAREN);
	vector params = {0};
	for (int i = 0; !match_token(p, TK_RPAREN); i++) {
		if (i)
			expect_token(p, TK_COMMA);
		tok = expect_token(p, TK_NAME);
		expect_token(p, TK_COLON);
		struct owo_param param;
		param.type = parse_type(p);
		vec_add(&p->mp, &params, &param, sizeof param);
	}
	expect_token(p, TK_COLON);
	owo_type ret = parse_type(p);
	fixed_buf body = parse_stmt_block(p);
	return decl_funcdef(name, ret, fb_shrink(&p->mp, params, sizeof (struct owo_param)), body);
}

