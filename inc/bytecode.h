#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>
#include <limits.h>
#include <assert.h>

#include "common.h"
#include "alloc.h"
#include "ast.h"


#define BC_INSTR_OPC 8 // (sizeof (int) * CHAR_BIT - 1 - __builtin_clz(BC_NUM))

typedef enum bc_opcode {
	BC_UNDEF,
	BC_RETI, // 1 operand: retval
} bc_opcode;

#define BC_NUM (BC_RETI + 1)

// layout will be more precise later
typedef struct bc_instr {
	bc_opcode opcode: BC_INSTR_OPC;
	int32_t operand: 32 - BC_INSTR_OPC;
} bc_instr;

static_assert(sizeof (bc_instr) == sizeof (uint32_t), "");

typedef struct bc_funcdef {
	ident_t name;
	small_buf insns; // INLINE bc_instr's
} bc_funcdef;

typedef struct bc_unit {
	small_buf funcs; // INLINE bc_funcdef's
	allocator al;
} bc_unit;

void bcu_init(bc_unit *u, allocator al);
void bcu_fini(bc_unit *u);

void bct_ast(bc_unit *u, owo_ast *ast);
bc_funcdef bct_func(bc_unit *u, struct owo_cfuncdef *owo_fn);
bc_instr bct_sreturn(bc_unit *u, struct owo_sreturn *stmt);

void bcu_dump(FILE *f, const bc_unit *u);
void bcu_dump_fn(FILE *f, const bc_funcdef *fn);

#endif /* BYTECODE_H */

