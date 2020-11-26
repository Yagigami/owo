#include <string.h>

#include "common.h"
#include "x86-64.h"


static byte_t *emit_n(gen_x64 *gen, const byte_t *mem, len_t n)
{
	byte_t *start = sm_reserve(gen->al, &gen->insns, n, sizeof *start);
	memcpy(start, mem, n);
	sm_add_len(&gen->insns, n);
	return start;
}

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
	gen_free(gen->al, sm_mem(gen->insns), sm_cap(gen->insns));
	gen_free(gen->al, sm_mem(gen->syms), sm_cap(gen->syms) * sizeof (byte_t *));
}

void gx64t_bc(gen_x64 *gen, bc_unit *u)
{
	struct cg_sym *syms = sm_fit(gen->al, &gen->syms, sm_len(u->funcs), sizeof (bc_funcdef));
	sm_add_len(&gen->syms, sm_len(u->funcs));
	sm_iter(bc_funcdef, u->funcs, fn, {
		len_t idx = sm_len(gen->insns);
		sm_iter(bc_instr, fn.insns, ins, switch (ins.opcode) {
			byte_t buf[16];
			len_t i;
		case BC_RETI: {
			i = gen_mov_imm(buf, 00, ins.operand);
			buf[i++] = 0xC3;
			emit_n(gen, buf, i);
			break;
			}
		case BC_UNDEF:
		default:
			__builtin_unreachable();
		});
		syms->idx  = idx;
		syms->size = sm_len(gen->insns) - idx;
		syms->name = fn.name;
		syms++;
	});
}

void gx64_dump(FILE *f, const gen_x64 *gen)
{
	sm_iter(const struct cg_sym, gen->syms, sym, {
		fprintf(f, "%s(%d:%d)\n", repr_ident(sym.name, NULL), sym.idx, sym.size);
	});
	int total = 0;
	static const int w = 20;
	sm_iter(const byte_t, gen->insns, byte, {
		if (total + 3 > w) {
			total = 0;
			fprintf(f, "\n");
		}
		fprintf(f, "%02x ", byte);
	});
}

