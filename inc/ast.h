#ifndef OWO_AST_H
#define OWO_AST_H

#include "begincpp.h"

#include "common.h"
#include "token.h"
#include "alloc.h"


typedef struct owo_ast owo_ast;

struct owo_ast {
	mem_arena ar;
	vector decls;
};

typedef enum owo_dkind owo_dbase;
typedef enum owo_skind owo_sbase;
typedef enum owo_ekind owo_ebase;
typedef enum owo_tkind owo_tbase;

// `ptr` points to the start of an ast node
// [20:64[ = ptr<<4
// [16:20[ = reserved(0)
// [ 8:16[ = owo_dbase
// [ 0: 8[ = owo_sbase
typedef uintptr_t owo_decl;
// [ 0: 8[ = owo_sbase
typedef uintptr_t owo_stmt;
// [ 8:16[ = owo_ebase
// [ 0: 8[ = owo_sbase
typedef uintptr_t owo_expr;
// [ 0: 8[ = owo_tbase
typedef uintptr_t owo_type;

enum owo_dkind {
	OWD_NONE = 0,
	OWD_FUNC,
	OWD_VAR,
	OWD_LABEL,
	OWD_NUM,
};

static_assert(OWD_NUM < 256, "");

enum owo_skind {
	OWS_NONE = 0,
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
	OWS_EXPR,
	OWS_DECL,
	OWS_NUM,
};

static_assert(OWS_NUM < 256, "");

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

static_assert(OWE_NUM < 256, "");

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

static_assert(OWT_NUM < 256, "");

struct owo_tptr {
	owo_type inner;
};

struct owo_sreturn {
	owo_expr rval;
};

struct owo_eint {
	uint64_t val;
};

struct owo_eident {
	ident_t ident;
};

struct owo_dfuncdef {
	ident_t name;
	owo_type ret;
	fixed_buf params; // INLINE struct owo_param
	fixed_buf body; // PTR
};

struct owo_dvar {
	ident_t name;
	owo_expr init;
	owo_type type;
};

struct owo_dlabel {
	ident_t name;
	owo_stmt stmt;
};

struct owo_param {
	ident_t name;
	owo_type type;
};

extern owo_type owo_tint;
extern owo_type owo_tchar;
extern owo_type owo_tbool;
extern owo_type owo_tfloat;

owo_type type_ptr(owo_type t);

owo_expr expr_int(uint64_t val);
owo_expr expr_ident(ident_t ident);
owo_expr expr_ternary(owo_expr cond_expr, owo_expr then_expr, owo_expr else_expr);
owo_expr expr_binary(token_kind op, owo_expr lhs, owo_expr rhs);
owo_expr expr_unary(token_kind op, owo_expr expr);

owo_decl decl_funcdef(ident_t name, owo_type ret, fixed_buf params, fixed_buf body);
owo_decl decl_var(ident_t name, owo_type type, owo_expr init);

owo_stmt stmt_sreturn(owo_expr rval);
owo_stmt stmt_decl(owo_decl decl);

void ast_init(owo_ast *ast);
void ast_fini(owo_ast *ast);

static inline void *astn_ptr(uintptr_t node) { return (void *) ((node >> 20) << 4); }
static inline int astn_base2(uintptr_t node) { return (node >> 8) & 0xFF; }
static inline int astn_base(uintptr_t node) { return node & 0xFF; }
static inline uintptr_t astn_set(int base, int base2, const void *ptr)
{
	assert(0 < base && base < 256);
	assert(0 <= base2 && base2 < 256);
	assert(((uintptr_t) ptr & 0xF) == 0 && ((uintptr_t) ptr >> 48) == 0);
	return base | (base2 << 8) | ((uintptr_t) ptr << 16);
}

#include "endcpp.h"

#endif /* OWO_AST_H */


