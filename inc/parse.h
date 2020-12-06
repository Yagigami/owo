#ifndef OWO_PARSER_H
#define OWO_PARSER_H

#include "begincpp.h"

#include "token.h"
#include "ast.h"
#include "alloc.h"


typedef struct parser {
	lexer l;
	owo_ast ast;
	multipool mp;
} parser;

void parser_init(parser *p, stream s, lex_str_func *on_str);
void parser_fini(parser *p);
void parse(parser *p);
owo_decl parse_func(parser *p);
owo_stmt parse_stmt(parser *p);
small_buf parse_stmt_block(parser *p);
owo_type parse_type(parser *p);

#define KEYWORDS() \
	X(else)		\
	X(for)		\
	X(func)		\
	X(if)		\
	X(int)		\
	X(return)	\
	X(var)		\
	X(while)

#define X(w) extern ident_t kw_ ## w;
KEYWORDS()
#undef X

#include "endcpp.h"

#endif /* OWO_PARSER_H */

