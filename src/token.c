#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "alloc.h"
#include "token.h"


hash_t identifier_hash(key_t k)
{
	ident_t id = (ident_t) k;
	hash_t h = 0;
	const char *restrict str = fb_mem(id);
	for (len_t i = 0, len = sm_len(id); i < len; i++)
		h += str[i] * (i + 1);
	return h;
}

int identifier_cmp(key_t k1, key_t k2)
{
	ident_t i1 = (ident_t) k1, i2 = (ident_t) k2;
	len_t l1 = fb_len(i1), l2 = fb_len(i2);
	int diff = l1 - l2;
	if (diff) return diff;
	return strncmp(fb_mem(i1), fb_mem(i2), l1);
}

void lexer_init(lexer *l, allocator al, stream s)
{
	l->str = s.buf;
	l->al = al;
#ifndef NDEBUG
	assert(s.buf[s.len] == '\0');
	memset(&l->tok, 0, sizeof l->tok);
#endif
	pmap_init(&l->ids, al);
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

void lexer_next(lexer *l)
{
space:  ;
	char *start = l->str;
	ident_t name;
	switch (*l->str) {
	case '\0':
		l->tok.kind = TK_EOF;
		return;
	case ' ': case '\t': case '\n': case '\f': case '\v':
		l->str = lex_space(l->str);
		goto space;
	case 'a' ... 'z': case 'A' ... 'Z': case '_':
		l->str = lex_name(l->str);
		if (l->str - start > 32) {
			fprintf(stderr, "name \"%.*s\" is too long.\n", (int) (l->str - start), start);
			__builtin_unreachable();
		}
		name = fb_set(l->str - start, start);
		l->tok.tid = *(ident_t *) pmap_intern(&l->ids, (void *) name, identifier_hash, identifier_cmp);
		l->tok.kind = TK_NAME;
		return;
	case '0' ... '9':
		l->tok.kind = TK_INT;
		l->tok.tint = *l->str++ - '0';
		while (isdigit(*l->str)) {
			l->tok.tint = (l->tok.tint * 10) + *l->str++ - '0';
		}
		break;
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
				(int) fb_len(t.tid),
				(int) fb_len(t.tid),
				(char *) fb_mem(t.tid));
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
	xfree(l->ids.mem);
	// we don't allocate for names so no need to free them
}
