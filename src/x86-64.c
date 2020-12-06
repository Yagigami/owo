#include <string.h>

#include "common.h"
#include "x86-64.h"


static len_t gen_mov_imm(byte_t *dst, int reg, uint64_t val)
{
	// ew
	if (val < BIT(32)) {
		int upper = reg >> 3;
		if (upper) *dst++ = 0x41;
		*dst++ = 0xB8 | (reg & 07);
		memcpy(dst, &val, 4);
		return 5 + upper;
	} else {
		*dst++ = 0x40 | (1 << 3) | (reg >> 3);
		*dst++ = 0xB8 | (reg & 07);
		memcpy(dst, &val, 8);
		return 10;
	}
}

void gx64_init(gen_x64 *gen, allocator al)
{
	gen->al = al;
	gen->insns = 0;
	gen->syms = 0;
}

void gx64_fini(gen_x64 *gen)
{
	gen_free(gen->al, fb_mem(gen->insns), fb_len(gen->insns));
	gen_free(gen->al, fb_mem(gen->syms), fb_len(gen->syms) * sizeof (byte_t *));
}

void gx64t_bc(gen_x64 *gen, bc_unit *u)
{
	len_t len = fb_len(u->funcs);
	struct cg_sym *syms = gen_alloc(gen->al, len * sizeof *syms);
	vector insns = {0};
	for (bc_funcdef *it_f = fb_mem(u->funcs), *end_f = it_f + len; it_f != end_f; it_f++) {
		bc_funcdef fn = *it_f;
		len_t idx = insns.len;
		for (bc_instr *it = fb_mem(fn.insns), *end = it + fb_len(fn.insns); it != end; it++) {
			bc_instr ins = *it;
			switch (ins.opcode) {
				byte_t buf[16];
				len_t i;
			case BC_RETI: {
				i = gen_mov_imm(buf, 00, ins.operand[0]);
				buf[i++] = 0xC3;
				vec_extend(gen->al, &insns, buf, i, 1);
				break;
			}
			case BC_SETI: {
				fatal_error(ERR_UNKNOWN, "unhandled yet!");
			}
			case BC_BIRTH : {
				fatal_error(ERR_UNKNOWN, "unhandled yet!");
			}
			case BC_DEATH : {
				fatal_error(ERR_UNKNOWN, "unhandled yet!");
			}
			case BC_UNDEF:
			default:
				__builtin_unreachable();
			}
		}
		syms->idx  = idx;
		syms->size = insns.len - idx;
		syms->name = fn.name;
		syms++;
	}
	gen->syms = fb_set(len, syms - len);
	gen->insns = fb_shrink(gen->al, insns, 1);
}

void gx64_dump(FILE *f, const gen_x64 *gen)
{
	for (struct cg_sym *it = fb_mem(gen->syms), *end = it + fb_len(gen->syms); it != end; it++) {
		struct cg_sym sym = *it;
		fprintf(f, "%.*s(%d:%d)\n", (int) ident_len(&sym.name), sym.name.buf, sym.idx, sym.size);
	}
	int total = 0;
	static const int w = 20;
	for (byte_t *it = fb_mem(gen->insns), *end = it + fb_len(gen->insns); it != end; it++) {
		byte_t byte = *it;
		if (total + 3 > w) {
			total = 0;
			fprintf(f, "\n");
		}
		fprintf(f, "%02x ", byte);
	}
}

