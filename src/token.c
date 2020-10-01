#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "token.h"


void lexer_init(lexer *l, stream s)
{
	l->str = s.buf;
#ifndef NDEBUG
	assert(s.buf[s.len] == '\0');
	memset(&l->tok, 0, sizeof l->tok);
#endif
}

void lexer_next(lexer *l)
{
space:
	switch (*l->str) {
	case ' ': case '\t': case '\n': case '\f': case '\v':
		while (isspace(*l->str)) l->str++;
		goto space;
	case '\0':
		l->tok.kind = TK_EOF;
		break;
	case 'a' ... 'z': case 'A' ... 'Z': case '_':
		break;
	default:
		assert(0);
	}
	l->str++;
}

void token_print(FILE *f, token t)
{
	switch (t.kind) {
	case TK_NONE:
		fprintf(f, "<unknown>\n");
		break;
	case TK_ASCII:
	case TK_END:
		fprintf(f, "<this shouldn't happpen\n");
		break;
	case TK_INT:
		fprintf(f, "int(%" PRIu64 ")\n", t.tint);
		break;
	case TK_NAME:
		fprintf(f, "id[%d](%.*s)\n",
				t.tid->len,
				t.tid->len,
				t.tid->str);
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
	}
}

