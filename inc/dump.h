#ifndef OWO_DUMP_H
#define OWO_DUMP_H

#include <stdio.h>

#include "bytecode.h"


void bcu_dump(FILE *f, const bc_unit *u);
void bcu_dump_fn(FILE *f, const bc_funcdef *fn);

#endif /* OWO_DUMP_H */

