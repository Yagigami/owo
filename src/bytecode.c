#include <assert.h>
#include <string.h>

#include "alloc.h"
#include "bytecode.h"


struct bc_entry {
	ident_t ident;
	bc_symbol sym;
};

struct bc_context {
	bc_funcdef func;
};

#define DEF_OPCODE(operands, op0_meaning, op1_meaning, op2_meaning) \
	(operands | \
	 (op0_meaning << (MAX_OPS + 0)) | \
	 (op1_meaning << (MAX_OPS + 1)) | \
	 (op2_meaning << (MAX_OPS + 2)))
#define DEF_OPCODE0() 0
#define DEF_OPCODE1(op0_meaning) DEF_OPCODE(1, op0_meaning, 0, 0)
#define DEF_OPCODE2(op0_meaning, op1_meaning) DEF_OPCODE(2, op0_meaning, op1_meaning, 0)
#define DEF_OPCODE3(op0_meaning, op1_meaning, op2_meaning) DEF_OPCODE(3, op0_meaning, op1_meaning, op2_meaning)
#define IMM 1
#define REF 0

const uint8_t opc2ops[BC_NUM] = {
	[BC_UNDEF         ] = DEF_OPCODE0(),
	[BC_RET           ] = DEF_OPCODE1(REF),
	[BC_SETI          ] = DEF_OPCODE2(REF, IMM),
	[BC_SET           ] = DEF_OPCODE2(REF, REF),
	[BC_FUNC_PROLOGUE ] = DEF_OPCODE0(),
	[BC_FUNC_EPILOGUE ] = DEF_OPCODE0(),
};

// bad ik
hash_t ident_hash(ident_t ident) 
{
	hash_t hash = 0;
	for (int i = 0; i < IDENT_MAXLEN; i++)
		hash += ident.buf[i] * i;
	return hash;
}

static len_t bct_eint(bc_unit *u, struct bc_context *ctx, vector *insns, struct owo_eint *eint)
{
	bc_instr ins[2];
	ins[0].opcode = BC_SETI;
	ins[0].operand[0] = ++ctx->func.n_locals;
	memcpy(ins + 1, &eint->val, sizeof *ins);
	vec_extend(u->al, insns, ins, sizeof ins / sizeof *ins, sizeof *ins);
	return ins[0].operand[0];
}

static cmp_func _ident_cmp;

int _ident_cmp(key_t _i1, key_t _i2)
{
	ident_t *i1 = _i1, *i2 = _i2;
	return ident_cmp(*i1, *i2);
}

static len_t bct_eident(bc_unit *u, struct bc_context *ctx, vector *insns, struct owo_eident *eident)
{
	(void) u, (void) insns;
	struct bc_entry *entry = pmap_find(&ctx->func.locals, ident_hash(eident->ident),
			&eident->ident, _ident_cmp, sizeof *entry);
	return entry->sym.id;
}

// TODO: make this return some bc_operand thing that can be an operand id or an embedded value
// 	 that would reduce the HUGE load/store/load/store/... mess
static len_t bct_expr(bc_unit *u, struct bc_context *ctx, vector *insns, owo_expr expr)
{
	switch (astn_base2(expr)) {
	case OWE_INT: {
		struct owo_eint *eint = astn_ptr(expr);
		return bct_eint(u, ctx, insns, eint);
	}
	case OWE_IDENT: {
		struct owo_eident *eident = astn_ptr(expr);
		return bct_eident(u, ctx, insns, eident);
	}
	default:
		__builtin_unreachable();
	}
}

static void bct_dvar(bc_unit *u, struct bc_context *ctx, vector *insns, struct owo_dvar *decl)
{
	struct bc_entry *entry = pmap_push(u->al, &ctx->func.locals, ident_hash(decl->name), sizeof *entry);
	entry->ident = decl->name;
	entry->sym.id = ctx->func.n_locals;
	assert(decl->type == owo_tint);
	bc_instr ins;
	ins.opcode = BC_SET;
	ins.operand[0] = entry->sym.id;
	ins.operand[1] = bct_expr(u, ctx, insns, decl->init);
	// keep track of a stack ?
	ctx->func.n_locals++;
	vec_add(u->al, insns, &ins, sizeof ins);
}

static void bct_sdecl(bc_unit *u, struct bc_context *ctx, vector *insns, owo_decl decl)
{
	switch (astn_base2(decl)) {
	case OWD_VAR: {
		struct owo_dvar *var = astn_ptr(decl);
		bct_dvar(u, ctx, insns, var);
		break;
	}
	default:
		__builtin_unreachable();
	}
}

static void bct_sreturn(bc_unit *u, struct bc_context *ctx, vector *insns, struct owo_sreturn *stmt)
{
	(void) ctx;
	assert(astn_base2(stmt->rval) == OWE_INT);
	bc_instr ins[2];
	// strange
	ins[0].opcode = BC_FUNC_EPILOGUE;
	ins[1].opcode = BC_RET;
	ins[1].operand[0] = bct_expr(u, ctx, insns, stmt->rval);
	vec_extend(u->al, insns, &ins, sizeof ins / sizeof *ins, sizeof *ins);
}

static bc_funcdef bct_func(bc_unit *u, struct owo_dfuncdef *owo_fn)
{
	struct bc_context ctx;
	ctx.func.name = owo_fn->name;
	pmap_init(&ctx.func.locals);
	ctx.func.n_locals = 0;
	vector insns = {0};
	bc_instr ins;
	ins.opcode = BC_FUNC_PROLOGUE;
	vec_add(u->al, &insns, &ins, sizeof ins);
	for (owo_stmt *start = fb_mem(owo_fn->body), *end = start + fb_len(owo_fn->body),
			*it_a = start; it_a != end; it_a++) {
		owo_stmt it = *it_a;
		switch (astn_base(it)) {
		case OWS_RETURN: {
			struct owo_sreturn *stmt = (struct owo_sreturn *) astn_ptr(it);
			bct_sreturn(u, &ctx, &insns, stmt);
			break;
		}
		case OWS_DECL: {
			owo_decl decl = (owo_decl) it;
			bct_sdecl(u, &ctx, &insns, decl);
			break;
		}
		default:
		       __builtin_unreachable();
		}
	}
	ctx.func.insns = fb_shrink(u->al, insns, sizeof (bc_instr));
	return ctx.func;
}

void bcu_init(bc_unit *u, allocator al)
{
#ifndef NDEBUG
	u->funcs = 0;
#endif
	u->al = al;
	pmap_init(&u->global_syms);
}

void bcu_fini(bc_unit *u)
{
	assert(fb_len(u->funcs) > 0);
	for (bc_funcdef *it = fb_mem(u->funcs), *end = it + fb_len(u->funcs); it != end; it++) {
		bc_funcdef fn = *it;
		pmap_fini(u->al, &fn.locals, sizeof (struct bc_entry));
		gen_free(u->al, fb_mem(fn.insns), fb_len(fn.insns) * sizeof (bc_instr));
	}
	gen_free(u->al, fb_mem(u->funcs), fb_len(u->funcs) * sizeof (bc_funcdef));
	if (u->global_syms.mem) pmap_fini(u->al, &u->global_syms, sizeof (struct bc_entry));
}

void bct_ast(bc_unit *u, owo_ast *ast)
{
	vector funcs = {0};
	for (owo_decl *start = ast->decls.mem, *end = start + ast->decls.len,
			*it_a = start; it_a != end; it_a++) {
		owo_decl it = *it_a;
		switch (astn_base2(it)) {
		case OWD_FUNC: {
		       struct owo_dfuncdef *decl = (struct owo_dfuncdef *) astn_ptr(it);
		       bc_funcdef fn = bct_func(u, decl);
		       vec_add(u->al, &funcs, &fn, sizeof fn);
		       break;
		}
		case OWD_NONE: case OWD_NUM:
		default:
		       assert(0);
		       __builtin_unreachable();
		}
	}
	u->funcs = fb_shrink(u->al, funcs, sizeof (bc_funcdef));
}
