#include <assert.h>
#include <string.h>

#include "alloc.h"
#include "bytecode.h"


struct bc_entry {
	ident_t ident;
	bc_symbol sym;
};

const uint8_t opc2ops[BC_NUM] = {
	[BC_UNDEF] = -1,
	[BC_RETI ] = 1,
	[BC_SETI ] = 2,
	[BC_BIRTH] = 1,
	[BC_DEATH] = 1,
};

// bad ik
hash_t ident_hash(ident_t ident) 
{
	hash_t hash = 0;
	for (int i = 0; i < IDENT_MAXLEN; i++)
		hash += ident.buf[i] * i;
	return hash;
}

static void bct_dvar(bc_unit *u, vector *insns, struct owo_dvar *decl)
{
	struct bc_entry *entry = pmap_push(u->al, &u->local_syms, ident_hash(decl->name), sizeof *entry);
	entry->ident = decl->name;
	entry->sym.id = u->local_syms.len - 1;
	assert(decl->type == owo_tint);
	assert(astn_base2(decl->init) == OWE_INT);
	bc_instr ins[2];
	ins[0].opcode = BC_BIRTH;
	ins[0].operand[0] = entry->sym.id;
	ins[1].opcode = BC_SETI;
	ins[1].operand[0] = ((struct owo_eint *) astn_ptr(decl->init))->val;
	vec_extend(u->al, insns, ins, sizeof ins / sizeof *ins, sizeof *ins);
}

static void bct_sdecl(bc_unit *u, vector *insns, owo_decl decl)
{
	switch (astn_base2(decl)) {
	case OWD_VAR: {
		struct owo_dvar *var = astn_ptr(decl);
		bct_dvar(u, insns, var);
		break;
	}
	case OWD_NONE: case OWD_NUM:
	default:
		__builtin_unreachable();
	}
}

static void bct_sreturn(bc_unit *u, vector *insns, struct owo_sreturn *stmt)
{
	assert(astn_base2(stmt->rval) == OWE_INT);
	bc_instr ins;
	ins.opcode = BC_RETI;
	ins.operand[0] = ((struct owo_eint *) astn_ptr(stmt->rval))->val;
	vec_add(u->al, insns, &ins, sizeof ins);
}

static bc_funcdef bct_func(bc_unit *u, struct owo_dfuncdef *owo_fn)
{
	bc_funcdef bc_fn;
	bc_fn.name = owo_fn->name;
	vector insns = {0};
	for (owo_stmt *start = fb_mem(owo_fn->body), *end = start + fb_len(owo_fn->body),
			*it_a = start; it_a != end; it_a++) {
		owo_stmt it = *it_a;
		switch (astn_base(it)) {
		case OWS_RETURN: {
			struct owo_sreturn *stmt = (struct owo_sreturn *) astn_ptr(it);
			bct_sreturn(u, &insns, stmt);
			break;
		}
		case OWS_DECL: {
			owo_decl decl = (owo_decl) it;
			bct_sdecl(u, &insns, decl);
			break;
		}
		case OWS_NONE: case OWS_NUM:
		default:
		       __builtin_unreachable();
		}
	}
	bc_fn.insns = fb_shrink(u->al, insns, sizeof (bc_instr));
	return bc_fn;
}

void bcu_dump(FILE *f, const bc_unit *u)
{
	for (bc_funcdef *fn = fb_mem(u->funcs), *end = fn + fb_len(u->funcs); fn != end; fn++) {
		bcu_dump_fn(f, fn);
	}
}

void bcu_dump_fn(FILE *f, const bc_funcdef *fn)
{
	fprintf(f, "fn(\"%.*s\")\n", (int) ident_len(&fn->name), fn->name.buf);
	for (bc_instr *it = fb_mem(fn->insns), *end = it + fb_len(fn->insns); it != end; it++) {
		bc_instr ins = *it;
		for (int n = opc2ops[ins.opcode], i = 0; i < n; i++) {
			uint8_t repr[4];
			memcpy(repr, ins.operand + i, sizeof repr);
			fprintf(f, " %02x %02x %02x %02x\n", repr[0], repr[1], repr[2], repr[3]);
		}
	}
	fprintf(f, "\n\n");
}

void bcu_init(bc_unit *u, allocator al)
{
#ifndef NDEBUG
	u->funcs = 0;
#endif
	u->al = al;
	pmap_init(&u->local_syms);
	pmap_init(&u->global_syms);
}

void bcu_fini(bc_unit *u)
{
	assert(fb_len(u->funcs) > 0);
	for (bc_funcdef *it = fb_mem(u->funcs), *end = it + fb_len(u->funcs); it != end; it++) {
		bc_funcdef fn = *it;
		gen_free(u->al, fb_mem(fn.insns), fb_len(fn.insns) * sizeof (bc_instr));
	}
	gen_free(u->al, fb_mem(u->funcs), fb_len(u->funcs) * sizeof (bc_funcdef));
	if (u->local_syms .mem) pmap_fini(u->al, &u->local_syms , sizeof (struct bc_entry));
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
