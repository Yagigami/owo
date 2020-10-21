#ifndef OWO_AST_H
#define OWO_AST_H

#include "begincpp.h"

#include "common.h"
#include "token.h"
#include "alloc.h"


typedef struct owo_ast owo_ast;

struct owo_ast {
	mem_arena ar;
	small_buf ctrs; // PTR
};

typedef struct owo_funcdef owo_funcdef;

typedef enum owo_ckind owo_cbase;
typedef enum owo_skind owo_sbase;
typedef enum owo_ekind owo_ebase;
typedef enum owo_tkind owo_tbase;

typedef owo_cbase *owo_construct;
typedef owo_sbase *owo_stmt;
typedef owo_ebase *owo_expr;
typedef owo_tbase *owo_type;

enum owo_ckind {
	OWC_NONE = 0,
	OWC_FUNC,
	OWC_NUM,
};

enum owo_skind {
	OWS_NONE = 0,
	OWS_VAR,
	OWS_BLK,
	OWS_BREAK,
	OWS_CONTINUE,
	OWS_GOTO,
	OWS_RETURN,
	OWS_IF,
	OWS_WHILE,
	OWS_FOR,
	OWS_DO,
	OWS_SWITCH,
	OWS_LABEL,
	OWS_EXPR,
	OWS_NUM,
};

enum owo_ekind {
	OWE_NONE = 0,
	OWE_INT,
	// OWE_STR,
	OWE_IDENT,
	OWE_TERNARY,
	OWE_BINARY,
	OWE_UNARY,
	// ...
	OWE_NUM,
};

enum owo_tkind {
	OWT_NONE = 0,
	OWT_INT,
	OWT_CHAR,
	OWT_BOOL,
	OWT_FLOAT,
	OWT_PTR,
	OWT_OPT_PTR,
	OWT_ARRAY,
	OWT_FUNC,
	OWT_NUM,
};

struct owo_tptr {
	owo_tbase base;
	owo_type inner;
};

struct owo_sreturn {
	owo_sbase base;
	owo_expr rval;
};

struct owo_eint {
	owo_ebase base;
	uint64_t val;
};

struct owo_eident {
	owo_ebase base;
	ident_t ident;
};

struct owo_cfuncdef {
	owo_cbase base;
	ident_t name;
	owo_type ret;
	small_buf params; // INLINE struct owo_param
	small_buf body; // PTR
};

struct owo_param {
	ident_t name;
	owo_type type;
};

extern owo_tbase owo_tint[1];
extern owo_tbase owo_tchar[1];
extern owo_tbase owo_tbool[1];
extern owo_tbase owo_tfloat[1];

owo_type owt_ptr(owo_type t);

owo_expr owe_int(uint64_t val);
owo_expr owe_ident(ident_t ident);
owo_expr owe_ternary(owo_expr cond_expr, owo_expr then_expr, owo_expr else_expr);
owo_expr owe_binary(token_kind op, owo_expr lhs, owo_expr rhs);
owo_expr owe_unary(token_kind op, owo_expr expr);

owo_construct owc_funcdef(allocator al, ident_t name, owo_type ret, small_buf params, small_buf body);

owo_stmt ows_sreturn(owo_expr rval);

void ast_init(owo_ast *ast);
void ast_fini(owo_ast *ast);

#include "endcpp.h"

#endif /* OWO_AST_H */


