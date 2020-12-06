#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "alloc.h"
#include "token.h"


void lexer_init(lexer *l, stream s, lex_str_func *on_str, void *ctx)
{
	l->str = s.buf;
	l->on_str = on_str;
	l->ctx = ctx;
#ifndef NDEBUG
	assert(s.buf[s.len] == '\0');
	memset(&l->tok, 0, sizeof l->tok);
#endif
}

static char *lex_name(const char *str)
{
	const char *cur = str;
	while (isalpha(*cur) || *cur == '_') cur++;
	return (char *) cur;
}

static char *lex_space(const char *str)
{
	while (isspace(*str)) str++;
	return (char *) str;
}

ident_t ident_from_string(const char *start, const char *end)
{
	ident_t id;
	char save[16];
	char *end_   = (char *) end  ;
	memcpy(save, end_, 16);
	memset(end_, 0, 16);
	memcpy(id.buf, start, 16);
	memcpy(end_, save, 16);
	return id;
}

void lex_str_default(lexer *l, char *start)
{
	l->tok.tid = ident_from_string(start, l->str);
	l->tok.kind = TK_NAME;
}

void lexer_next(lexer *l)
{
space:  ;
	char *start = l->str;
	switch (*l->str) {
	case '\0':
		l->tok.kind = TK_EOF;
		return;
	case ' ': case '\t': case '\n': case '\f': case '\v':
		l->str = lex_space(l->str);
		goto space;
	// those that do full lexing don't need to increment `l->str` so they should return immediately
	case 'a' ... 'z': case 'A' ... 'Z': case '_':
		l->str = lex_name(l->str);
		if (l->str - start > 32) {
			fprintf(stderr, "name \"%.*s\" is too long.\n", (int) (l->str - start), start);
			__builtin_unreachable();
		}
		l->on_str(l, start);
		return;
	case '0' ... '9':
		l->tok.kind = TK_INT;
		l->tok.tint = *l->str++ - '0';
		while (isdigit(*l->str)) {
			l->tok.tint = (l->tok.tint * 10) + *l->str++ - '0';
		}
		return;
	case '/':
		if (*++l->str == '*') {
			while (l->str[0] != '*' && l->str[1] != '/') l->str++;
			goto space;
		} // ok there is /* but also: /= // /
		break;
#define CASE1(x) case x: l->tok.kind = *l->str; break
	CASE1('(');
	CASE1(')');
	CASE1(':');
	CASE1(';');
	CASE1('=');
	CASE1('{');
	CASE1('}');
	CASE1(',');
	CASE1('@');
	CASE1('?');
#undef CASE1
	default:
		fprintf(stderr, "unknown character `%c` (%d)\n", *l->str, *l->str);
		assert(0);
	}
	l->str++;
}

void token_print(FILE *f, token t)
{
	switch (t.kind) {
	case TK_ASCII:
	case TK_END:
		fprintf(f, "<this shouldn't happpen\n");
		break;
	case TK_INT:
		fprintf(f, "int(%" PRIu64 ")\n", t.tint);
		break;
	case TK_NAME:
		fprintf(f, "id[%d](%.*s)\n",
				(int) ident_len(&t.tid),
				(int) ident_len(&t.tid),
				t.tid.buf);
		break;
	case TK_STR:
		fprintf(f, "str[%d](%.*s)\n",
				(int) (t.tstr[1] - t.tstr[0]),
				(int) (t.tstr[1] - t.tstr[0]),
				t.tstr[0]);
		break;
	case TK_EOF:
		fprintf(f, "<EOF>\n");
		break;
	default:
		fprintf(f, "%c\n", t.kind);
		break;
	}
}

void lexer_fini(lexer *l)
{
	(void) l;
}
