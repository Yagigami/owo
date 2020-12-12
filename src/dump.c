#include "dump.h"
#include "bytecode.h"


static const char *bc_opc2str[BC_NUM] = {
	[BC_UNDEF] = "ud0",
	[BC_RET  ] = "ret",
	[BC_SETI ] = "seti",
	[BC_SET  ] = "set",
	[BC_FUNC_PROLOGUE] = "prologue",
	[BC_FUNC_EPILOGUE] = "epilogue",
};


void bcu_dump(FILE *f, const bc_unit *u)
{
	for (bc_funcdef *fn = fb_mem(u->funcs), *end = fn + fb_len(u->funcs); fn != end; fn++) {
		bcu_dump_fn(f, fn);
	}
}

void bcu_dump_fn(FILE *f, const bc_funcdef *fn)
{
	fprintf(f, "fn(\"%.*s\")\n", (int) ident_len(&fn->name), fn->name.buf);
	for (bc_instr *start = fb_mem(fn->insns), *end = start + fb_len(fn->insns), *it = start; it != end; it++) {
		bc_instr ins = *it;
		fprintf(f, "  %04lx:\t%-5s ", it - start, bc_opc2str[ins.opcode]);
		int info = opc2ops[ins.opcode];
		int imm_idx = 0;
		for (int i = 0; i < (info & 3); i++) {
			if (info & (1 << (MAX_OPS + i))) {
				uint32_t imm;
				memcpy(&imm, it + ++imm_idx, sizeof imm);
				fprintf(f, " #%x", imm);
			} else {
				fprintf(f, " %%%x", ins.operand[i]);
			}
		}
		fprintf(f, "\n");
		it += imm_idx;
	}
	fprintf(f, "\n\n");
}

