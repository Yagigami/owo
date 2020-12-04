#ifndef TOKEN_H
#define TOKEN_H

#include "begincpp.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#include "common.h"
#include "ptrmap.h"


typedef enum token_kind {
	TK_EOF = 0,

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

	TK_END,
} token_kind;

typedef struct token {
	union {
		uint64_t tint;
		ident_t tid;
		char (*tstr) [2];
	};
	token_kind kind;
} token;

typedef struct lexer lexer;
typedef void lex_str_func(lexer *l, char *start);

struct lexer {
	token tok;
	char *str;
	lex_str_func *on_str;
	void *ctx;
};

lex_str_func lex_str_default;
void lexer_init(lexer *l, stream s, lex_str_func *on_str, void *ctx);
void lexer_next(lexer *l);
void token_print(FILE *f, token t);
void lexer_fini(lexer *l);

// dangerous, temporarily overwrites end[0:15]
// 	start[0:15] valid read
// 	end  [0:15] valid read/write
ident_t ident_from_string(const char *start, const char *end);

#include "endcpp.h"

#endif /* TOKEN_H */

