#ifndef CODEGEN_X86_64_H
#define CODEGEN_X86_64_H

#include "common.h"
#include "bytecode.h"
#include "alloc.h"


struct cg_sym {
	ident_t name;
	int32_t idx;
	int32_t size;
};

typedef struct {
	fixed_buf insns; // INLINE byte_t[]
	fixed_buf syms; // INLINE cg_sym (index bc pointers unstable)
	allocator al;
} gen_x64;

void gx64_init(gen_x64 *gen, allocator al);
void gx64_fini(gen_x64 *gen);
void gx64_dump(FILE *f, const gen_x64 *gen);

void gx64t_bc(gen_x64 *gen, bc_unit *u);

#endif /* CODEGEN_X86_64_H */

