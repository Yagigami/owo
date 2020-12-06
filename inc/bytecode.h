#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>
#include <limits.h>
#include <assert.h>

#include "common.h"
#include "alloc.h"
#include "ast.h"


#define BC_INSTR_OPC 8 // (sizeof (int) * CHAR_BIT - 1 - __builtin_clz(BC_NUM))
#define MAX_OPS 2

typedef enum bc_opcode {
	BC_UNDEF,
	BC_RETI, // 1 operand: retval
	BC_SETI, // 2 operands: dest, imm
	BC_BIRTH, // 1 operand: id
	BC_DEATH, // 1 operand: id
} bc_opcode;

#define BC_NUM (BC_DEATH + 1)

extern const uint8_t opc2ops[BC_NUM];

// layout will be more precise later
typedef struct bc_instr {
	bc_opcode opcode;
	int32_t operand[MAX_OPS];
} bc_instr;

typedef struct bc_funcdef {
	ident_t name;
	fixed_buf insns; // INLINE bc_instr's
} bc_funcdef;

typedef struct bc_unit {
	fixed_buf funcs; // INLINE bc_funcdef's
	ptrmap local_syms;
	ptrmap global_syms;
	allocator al;
} bc_unit;

typedef struct bc_symbol {
	int id;
} bc_symbol;

void bcu_init(bc_unit *u, allocator al);
void bcu_fini(bc_unit *u);

void bct_ast(bc_unit *u, owo_ast *ast);

void bcu_dump(FILE *f, const bc_unit *u);
void bcu_dump_fn(FILE *f, const bc_funcdef *fn);

#endif /* BYTECODE_H */

