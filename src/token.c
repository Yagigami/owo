#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "alloc.h"
#include "token.h"


#define KEYWORDS() \
	X(else) \
	X(for) \
	X(func) \
	X(if) \
	X(int) \
	X(let) \
	X(while) \

#define X(w) ident_t kw_ ## w;
KEYWORDS()
#undef X

static ptrmap keywords;

hash_func identifier_hash;
hash_t identifier_hash(key_t k)
{
	identifier *id = k;
	hash_t h = 0;
	for (int8_t i = 0; i < id->len; i++)
		h += id->str[i] * (i + 1);
	return h;
}

cmp_func identifier_cmp;
int identifier_cmp(key_t k1, key_t k2)
{
	ident_t i1 = k1, i2 = k2;
	int diff = i1->len - i2->len;
	if (diff) return diff;
	return strncmp(i1->str, i2->str, i1->len);
}

__attribute__((constructor))
void keywords_init(void)
{
	pmap_reserve(&keywords, 7);
#define X(w) static identifier i ## w = { sizeof # w - 1, # w };
	KEYWORDS()
#undef X

#define X(w) kw_ ## w = *pmap_push(&keywords, &i ## w, identifier_hash);
	KEYWORDS()
#undef X
}

void lexer_init(lexer *l, stream s)
{
	l->str = s.buf;
#ifndef NDEBUG
	assert(s.buf[s.len] == '\0');
	memset(&l->tok, 0, sizeof l->tok);
#endif
	memset(&l->ids, 0, sizeof l->ids);
}

bool is_keyword(ident_t id)
{
	return (bool) pmap_find(&keywords, (key_t) id, identifier_hash, identifier_cmp);
}

void lexer_next(lexer *l)
{
space:
	;
	char *start = l->str;
	switch (*l->str) {
		ident_t *id;
		identifier *name;
	case ' ': case '\t': case '\n': case '\f': case '\v':
		while (isspace(*l->str)) l->str++;
		goto space;
	case '\0':
		l->tok.kind = TK_EOF;
		break;
	case 'a' ... 'z': case 'A' ... 'Z': case '_':
		while (isalpha(*l->str) || *l->str == '_') l->str++;
		name = xmalloc(sizeof *name + (l->str - start));
		name->len = l->str - start;
		memcpy(name->str, start, name->len);
		id = (ident_t *) pmap_find(&keywords, name, identifier_hash, identifier_cmp);
		if (id) {
			l->tok.kind = TK_KW;
			l->tok.tid = *id;
			xfree(name);
		} else {
			l->tok.kind = TK_NAME;
			// may already be in the map
			l->tok.tid = *pmap_push(&l->ids, name, identifier_hash);
		}
		break;
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
		}
		break;
#define CASE1(x) case x: l->tok.kind = *l->str++; break
	CASE1('(');
	CASE1(')');
	CASE1(':');
	CASE1(';');
	CASE1('=');
	CASE1('{');
	CASE1('}');
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
	case TK_KW:
		if (0);
#define X(w) \
		else if (t.tid == kw_ ## w) fprintf(f, "kw(" # w ")\n");
		KEYWORDS()
#undef X
		else
			fprintf(f, "kw(<unknown>)\n");
		break;
	case TK_EOF:
		fprintf(f, "<EOF>\n");
		break;
	}
}

void lexer_fini(lexer *l)
{
	uint8_t *meta = l->ids.mem;
	key_t *buf = (key_t *) ((char *) l->ids.mem + (1 << l->ids.log2_cap));
	for (len_t i = 0; i < (1 << l->ids.log2_cap); i++) {
		if ((meta[i] & 0x80))
			xfree(buf[i]);
	}
}
