#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#include "common.h"
#include "ptrmap.h"


typedef enum token_kind {
	TK_NONE = 0,

	TK_EXCLM = '!',
	TK_DQUOTE = '"',
	TK_HASH = '#',
	TK_DOLLAR = '$',
	TK_PERCENT = '%',
	TK_AMPRSD = '&',
	TK_SQUOTE = '\'',
	TK_LPAREN = '(',
	TK_RPAREN = ')',
	TK_STAR = '*',
	TK_PLUS = '+',
	TK_COMMA = ',',
	TK_HYPHEN = '-',
	TK_DOT = '.',
	TK_FSLASH = '/',
	TK_COLON = ':',
	TK_SCOLON = ';',
	TK_LT = '<',
	TK_EQ = '=',
	TK_GT = '>',
	TK_QUEST = '?',
	TK_AT = '@',
	TK_LBRACK = '[',
	TK_BSLASH = '\\',
	TK_RBRACK = ']',
	TK_CARET = '^',
	TK_UNDERSC = '_',
	TK_BTICK = '`',
	TK_LBRACE = '{',
	TK_BAR = '|',
	TK_RBRACE = '}',

	TK_ASCII = 127,
	TK_INT,
	TK_NAME,
	TK_STR,
	TK_KW,

	TK_EOF,
	TK_END,
} token_kind;

typedef struct token {
	union {
		uint64_t tint;
		ident_t tid;
		char (*tstr) [2];
	};
	token_kind kind: sizeof (int) * CHAR_BIT - 8;
	bool is_operator: 1;
	bool _1         : 1;
	bool _2         : 1;
	bool _3         : 1;
	bool _4         : 1;
	bool _5         : 1;
	bool _6         : 1;
	bool _7         : 1;
} token;

extern ident_t kw_else;
extern ident_t kw_func;
extern ident_t kw_if;
extern ident_t kw_int;
extern ident_t kw_let;
extern ident_t kw_while;

typedef struct lexer {
	ptrmap ids;
	char *str;
	token tok;
} lexer;

void lexer_init(lexer *l, stream s);
void lexer_next(lexer *l);
void token_print(FILE *f, token t);
void lexer_fini(lexer *l);

#endif /* TOKEN_H */

