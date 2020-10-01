#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#include "common.h"


typedef enum token_kind {
	TK_NONE = 0,

	TK_ASCII = 127,
	TK_INT,
	TK_NAME,
	TK_STR,

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
	bool is_keyword : 1;
	bool _2         : 1;
	bool _3         : 1;
	bool _4         : 1;
	bool _5         : 1;
	bool _6         : 1;
	bool _7         : 1;
} token;

typedef struct lexer {
	char *str;
	token tok;
} lexer;

void lexer_init(lexer *l, stream s);
void lexer_next(lexer *l);
void token_print(FILE *f, token t);

#endif /* TOKEN_H */

