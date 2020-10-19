#include <string.h>
#include <assert.h>

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

void parser_init(parser *p, stream s)
{
	lexer_init(&p->l, s);
	ast_init(&p->ast);
	mp_init(&p->mp, 8, 4096, 16 * 1024);
	init_keywords(&p->l.ids);
	memset(&p->ast, 0, sizeof p->ast);
}

void parser_fini(parser *p)
{
	lexer_fini(&p->l);
	ast_fini(&p->ast);
	mp_fini(&p->mp);
}

void parse(parser *p)
{
	small_buf ctrs = {0};
	while (1) {
		lexer_next(&p->l);
		if (p->l.tok.tid == kw_func)
			sm_add(&p->mp, &ctrs, parse_func(p), PTRSZ);
		else {
			assert(p->l.tok.kind == TK_EOF);
			return;
		}
	}
}

owo_type parse_type(parser *p)
{
	assert(p->l.tok.kind == TK_NAME);
	assert(p->l.tok.tid == kw_int);
	lexer_next(&p->l);
	owo_type t = owo_tint;
	while (p->l.tok.kind == TK_AT) {
		lexer_next(&p->l);
		t = owt_ptr(t);
	}
	return t;
}

owo_expr parse_expr(parser *p)
{
	assert(p->l.tok.kind == TK_INT);
	return owe_int(p->l.tok.tint);
}

owo_stmt parse_stmt(parser *p)
{
	assert(p->l.tok.kind == TK_NAME);
	assert(p->l.tok.tid == kw_return);
	lexer_next(&p->l);
	assert(p->l.tok.kind == TK_EQ);
	lexer_next(&p->l);
	owo_expr rval = parse_expr(p);
	return ows_sreturn(rval);
}

small_buf parse_stmt_block(parser *p)
{
	small_buf body = 0;
	assert(p->l.tok.kind == TK_LBRACE);
	while (lexer_next(&p->l), p->l.tok.kind != TK_RBRACE) {
		owo_stmt stmt = parse_stmt(p);
		sm_add(&p->mp, &body, &stmt, PTRSZ);
	}
	lexer_next(&p->l);
	return body;
}

owo_construct parse_func(parser *p)
{
	assert(p->l.tok.kind == TK_NAME);
	assert(p->l.tok.tid == kw_func);
	lexer_next(&p->l);
	assert(p->l.tok.kind == TK_NAME);
	ident_t *name = (ident_t *) pmap_push(&p->l.ids, (void *) p->l.tok.tid, identifier_hash);
	lexer_next(&p->l);
	assert(p->l.tok.kind == TK_LPAREN);
	lexer_next(&p->l);
	small_buf params = 0;
	for (int i = 0; p->l.tok.kind != TK_RPAREN; i++) {
		if (i) {
			assert(p->l.tok.kind == TK_COMMA);
			lexer_next(&p->l);
		}
		struct owo_param param;
		assert(p->l.tok.kind == TK_NAME);
		param.name = *(ident_t *) pmap_push(&p->l.ids, (void *) p->l.tok.tid, identifier_hash);
		lexer_next(&p->l);
		assert(p->l.tok.kind == TK_COLON);
		lexer_next(&p->l);
		param.type = parse_type(p);
		sm_add(&p->mp, &params, &param, sizeof param);
	}
	lexer_next(&p->l);
	assert(p->l.tok.kind == TK_COLON);
	lexer_next(&p->l);
	owo_type ret = parse_type(p);
	small_buf body = parse_stmt_block(p);
	return owc_funcdef(&p->mp, *name, ret, params, body);
}

