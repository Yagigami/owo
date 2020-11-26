#ifndef ELF_SERIALIZE_H
#define ELF_SERIALIZE_H

#include "x86-64.h"
#include "common.h"
#include "alloc.h"


stream elf_serialize_x64(allocator al, const gen_x64 *gen, const char *file);

#endif /* ELF_SERIALIZE_H */

