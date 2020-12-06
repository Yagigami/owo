#include "common.h"
#include "ast.h"
#include "token.h"
#include "alloc.h"


owo_type owo_tint = OWT_INT;
owo_type owo_tchar = OWT_CHAR;
owo_type owo_tbool = OWT_BOOL;
owo_type owo_tfloat = OWT_FLOAT;

// TODO: make this local
static thread_local mem_arena *cur_ar;

void ast_init(owo_ast *ast)
{
	ast->decls.mem = NULL;
	ast->decls.cap = 0;
	ast->decls.len = 0;
	arena_init(&ast->ar, &system_allocator, 1024 * 1024);
	cur_ar = &ast->ar;
}

void ast_fini(owo_ast *ast)
{
	arena_fini(&ast->ar);
	cur_ar = NULL;
}

static void *ast_alloc(len_t sz)
{
	return arena_alloc(cur_ar, ALIGN_UP_P2(sz, 16));
}

static owo_expr ast_expr(const void *expr, owo_ebase base) { return astn_set(OWS_EXPR, base, expr); }
static owo_stmt ast_stmt(const void *stmt, owo_sbase base) { return astn_set(base, 0, stmt); }
static owo_decl ast_decl(const void *decl, owo_dbase base) { return astn_set(OWS_DECL, base, decl); }
static owo_type ast_type(const void *type, owo_tbase base) { return astn_set(base, 0, type); }

owo_expr expr_int(uint64_t val)
{
	struct owo_eint *expr = ast_alloc(sizeof *expr);
	expr->val = val;
	return ast_expr(expr, OWE_INT);
}

owo_expr expr_ident(ident_t ident)
{
	struct owo_eident *expr = ast_alloc(sizeof *expr);
	expr->ident = ident;
	return ast_expr(expr, OWE_IDENT);
}


owo_decl decl_funcdef(ident_t name, owo_type ret, fixed_buf params, fixed_buf body)
{
	struct owo_dfuncdef *decl = ast_alloc(sizeof *decl);
	decl->name = name;
	decl->ret = ret;
	decl->params = params;
	decl->body = body;
	return ast_decl(decl, OWD_FUNC);
}

owo_stmt stmt_sreturn(owo_expr rval)
{
	struct owo_sreturn *stmt = ast_alloc(sizeof *stmt);
	stmt->rval = rval;
	return ast_stmt(stmt, OWS_RETURN);
}

owo_decl decl_var(ident_t name, owo_type type, owo_expr init)
{
	struct owo_dvar *decl = ast_alloc(sizeof *decl);
	decl->name = name;
	decl->type = type;
	decl->init = init;
	return ast_decl(decl, OWD_VAR);
}

owo_type type_ptr(owo_type t)
{
	struct owo_tptr *type = ast_alloc(sizeof *type);
	type->inner = t;
	return ast_type(type, OWT_PTR);
}

owo_stmt stmt_decl(owo_decl decl)
{
	return (owo_stmt) decl;
}

