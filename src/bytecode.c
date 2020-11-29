#include <assert.h>
#include <string.h>

#include "alloc.h"
#include "bytecode.h"


void bcu_dump(FILE *f, const bc_unit *u)
{
	sm_iter(bc_funcdef, u->funcs, fn, {
		bcu_dump_fn(f, &fn);
	});
}

void bcu_dump_fn(FILE *f, const bc_funcdef *fn)
{
	fprintf(f, "fn(\"%.*s\")\n", (int) ident_len(&fn->name), fn->name.buf);
	sm_iter(bc_instr, fn->insns, ins, {
		uint8_t repr[4];
		memcpy(repr, &ins, sizeof ins);
		fprintf(f, " %02x %02x %02x %02x\n", repr[0], repr[1], repr[2], repr[3]);
	});
	fprintf(f, "\n\n");
}

void bcu_init(bc_unit *u, allocator al)
{
	u->funcs = 0;
	u->al = al;
}

void bcu_fini(bc_unit *u)
{
	assert(sm_len(u->funcs) > 0);
	sm_iter(bc_funcdef, u->funcs, fn, {
		gen_free(u->al, sm_mem(fn.insns), sm_len(fn.insns) * sizeof (bc_instr));
	});
	gen_free(u->al, sm_mem(u->funcs), sm_cap(u->funcs) * sizeof (bc_funcdef));
}

void bct_ast(bc_unit *u, owo_ast *ast)
{
	sm_iter(owo_construct, ast->ctrs, it, switch (*it) {
	case OWC_FUNC: {
		struct owo_cfuncdef *ctr = (struct owo_cfuncdef *) it;
		bc_funcdef fn = bct_func(u, ctr);
		sm_add(u->al, &u->funcs, &fn, sizeof fn);
		break;
		}
	case OWC_NONE: case OWC_NUM:
	default:
		__builtin_unreachable();
	});
}

bc_funcdef bct_func(bc_unit *u, struct owo_cfuncdef *owo_fn)
{
	bc_funcdef bc_fn = { .insns = 0, .name = owo_fn->name, };
	sm_iter(owo_stmt, owo_fn->body, it, switch (*it) {
	case OWS_RETURN: {
		struct owo_sreturn *stmt = (struct owo_sreturn *) it;
		bc_instr ins = bct_sreturn(u, stmt);
		sm_add(u->al, &bc_fn.insns, &ins, sizeof ins);
		return bc_fn;
		}
	case OWS_VAR: {
		fatal_error(ERR_UNKNOWN, "variables are not handled yet!");
	}
	case OWS_NONE: case OWS_NUM:
	default:
		__builtin_unreachable();
	});
	__builtin_unreachable();
}

bc_instr bct_sreturn(bc_unit *u, struct owo_sreturn *stmt)
{
	(void) u;
	assert(*stmt->rval == OWE_INT);
	bc_instr ins = { .opcode = BC_RETI, .operand = ((struct owo_eint *) stmt->rval) ->val, };
	return ins;
}

