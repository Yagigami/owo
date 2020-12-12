#include <string.h>

#include "common.h"
#include "x86-64.h"


enum {
	RAX,
	RDX,
	RCX,
	RBX,
	RSP,
	RBP,
	RSI,
	RDI,
	R8,
	R9,
	R10,
	R11,
	R12,
	R13,
	R14,
	R15,
	R_END,
};

static byte_t x64_rex(int w, int r, int x, int b) { return 0x40 | (w << 3) | (r << 2) | (x << 1) | b; }
static byte_t x64_modrm(int mod, int rm, int reg) { return (mod << 6) | (reg << 3) | rm; }
static byte_t x64_sib(int ss, int idx, int base) { return (ss << 6) | (idx << 3) | base; }

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

static len_t gen_sub_imm(byte_t *dst, int reg, uint32_t val)
{
	*dst++ = x64_rex(1, 0, 0, reg >> 3);
	*dst++ = 0x81;
	*dst++ = x64_modrm(0b11, reg & 0b111, 05);
	memcpy(dst, &val, sizeof val);
	return 7;
}

static len_t gen_add_imm(byte_t *dst, int reg, uint32_t val)
{
	*dst++ = x64_rex(1, 0, 0, reg >> 3);
	*dst++ = 0x81;
	*dst++ = x64_modrm(0b11, reg & 0b111, 00);
	memcpy(dst, &val, sizeof val);
	return 7;
}

static len_t gen_mov_mr_rsp(byte_t *dst, uint8_t offset, int reg)
{
	*dst++ = x64_rex(1, reg >> 3, 0, 0);
	*dst++ = 0x89;
	*dst++ = x64_modrm(0b01, RSP, reg & 0b111);
	*dst++ = x64_sib(0, RSP, RSP);
	*dst++ = offset;
	return 5;
}

static len_t gen_mov_rm_rsp(byte_t *dst, int reg, uint8_t offset)
{
	*dst++ = x64_rex(1, reg >> 3, 0, 0);
	*dst++ = 0x8B;
	*dst++ = x64_modrm(0b01, RSP, reg & 0b111);
	*dst++ = x64_sib(0, RSP, RSP);
	*dst++ = offset;
	return 5;
}

static len_t gen_mov_rr(byte_t *buf, int dst, int src)
{
	*buf++ = x64_rex(1, src >> 3, 0, dst >> 3);
	*buf++ = 0x89;
	*buf++ = x64_modrm(0b11, dst & 0b111, src & 0b111);
	return 3;
}

static len_t gen_mov_mi_rsp(byte_t *buf, uint8_t offset, uint32_t val)
{
	*buf++ = x64_rex(1, 0, 0, 0);
	*buf++ = 0xC7;
	*buf++ = x64_modrm(0b01, RSP, 0);
	*buf++ = x64_sib(0, RSP, RSP);
	*buf++ = offset;
	memcpy(buf, &val, sizeof val);
	return 9;
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

static void test_gen(gen_x64 *gen, vector *insns)
{
	return;
	byte_t buf[4096];
	byte_t *cur = buf;

	for (int i = RAX; i < R_END; i++) {
		cur += gen_mov_imm(cur, i, 0x01234567);
		cur += gen_mov_rm_rsp(cur, i, 0x55);
		cur += gen_mov_mr_rsp(cur, 0x33, i);
		cur += gen_mov_mi_rsp(cur, 0x77, 0x89ABCDEF);
	}

	vec_extend(gen->al, insns, buf, cur - buf, 1);
}

static len_t gx64t_loc_offset(gen_x64 *gen, uint8_t operand)
{
	return operand * 8;
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
			len_t imm_idx = 0;
			bc_instr ins = *it;
			byte_t buf[32];
			len_t i;
			uint32_t imm;
			switch (ins.opcode) {
			case BC_FUNC_PROLOGUE: {
				i = gen_sub_imm(buf, RSP, fn.n_locals * 8);
				break;
			}
			case BC_FUNC_EPILOGUE:
				i = gen_add_imm(buf, RSP, fn.n_locals * 8);
				break;
			case BC_RET: {
				i = gen_mov_rm_rsp(buf, RAX, gx64t_loc_offset(gen, ins.operand[0] - fn.n_locals));
				buf[i++] = 0xC3;
				break;
			}
			case BC_SETI: {
				memcpy(&imm, it + ++imm_idx, sizeof imm);
				i = gen_mov_mi_rsp(buf, gx64t_loc_offset(gen, ins.operand[0]), imm);
				break;
			}
			case BC_SET:
				i = gen_mov_rm_rsp(buf, RAX, gx64t_loc_offset(gen, ins.operand[1]));
				i+= gen_mov_mr_rsp(buf + i, gx64t_loc_offset(gen, ins.operand[0]), RAX);
				break;
			case BC_UNDEF:
			default:
				__builtin_unreachable();
			}
			vec_extend(gen->al, &insns, buf, i, 1);
			it += imm_idx;
		}
		test_gen(gen, &insns);
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

