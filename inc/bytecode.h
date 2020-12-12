#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>
#include <limits.h>
#include <assert.h>

#include "common.h"
#include "alloc.h"
#include "ast.h"


#define BC_INSTR_OPC 8
#define MAX_OPS 2

typedef enum bc_opcode {
	BC_UNDEF,
	BC_RET, // 1 operand: rval
	BC_SETI, // 2 operands: dest, imm
	BC_SET, // 2 operands: dest, src
	BC_FUNC_PROLOGUE, // 0 operand
	BC_FUNC_EPILOGUE, // 0 operand
	BC_NUM,
} bc_opcode;

extern const uint8_t opc2ops[BC_NUM];

// layout will be more precise later
typedef struct bc_instr {
	bc_opcode opcode: BC_INSTR_OPC;
	uint8_t operand[MAX_OPS];
} bc_instr;

static_assert(sizeof (bc_instr) == sizeof (uint32_t), "");

typedef struct bc_funcdef {
	ident_t name;
	fixed_buf insns; // INLINE bc_instr's
	ptrmap locals;
	len_t n_locals;
} bc_funcdef;

typedef struct bc_unit {
	fixed_buf funcs; // INLINE bc_funcdef's
	ptrmap global_syms;
	allocator al;
} bc_unit;

typedef struct bc_symbol {
	int id;
} bc_symbol;

void bcu_init(bc_unit *u, allocator al);
void bcu_fini(bc_unit *u);

void bct_ast(bc_unit *u, owo_ast *ast);

#endif /* BYTECODE_H */

