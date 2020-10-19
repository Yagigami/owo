#include "common.h"
#include "ast.h"
#include "token.h"
#include "alloc.h"


owo_tbase owo_tint[1] = { OWT_INT };
owo_tbase owo_tchar[1] = { OWT_CHAR };
owo_tbase owo_tbool[1] = { OWT_BOOL };
owo_tbase owo_tfloat[1] = { OWT_FLOAT };

static void *ast_alloc(len_t sz)
{
	return xmalloc(sz);
}

owo_expr owe_int(uint64_t val)
{
	struct owo_eint *expr = ast_alloc(sizeof *expr);
	expr->base = OWE_INT;
	expr->val = val;
	return &expr->base;
}

owo_expr owe_ident(ident_t ident)
{
	struct owo_eident *expr = ast_alloc(sizeof *expr);
	expr->base = OWE_IDENT;
	expr->ident = ident;
	return &expr->base;
}


owo_construct owc_funcdef(ident_t name, owo_type ret, small_buf params, small_buf body)
{
	struct owo_cfuncdef *ctr = ast_alloc(sizeof *ctr);
	ctr->base = OWC_FUNC;
	ctr->name = name;
	ctr->ret = ret;
	sm_shrink_into(&system_allocator, &ctr->params, params, sizeof (struct owo_param));
	sm_shrink_into(&system_allocator, &ctr->body, body, PTRSZ);
	return &ctr->base;
}

owo_stmt ows_sreturn(owo_expr rval)
{
	struct owo_sreturn *stmt = ast_alloc(sizeof *stmt);
	stmt->base = OWS_RETURN;
	stmt->rval = rval;
	return &stmt->base;
}

owo_type owt_ptr(owo_type t)
{
	struct owo_tptr *type = ast_alloc(sizeof *type);
	type->base = OWT_PTR;
	type->inner = t;
	return &type->base;
}

