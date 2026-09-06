#ifndef CODEGEN_H
#define CODEGEN_H

#include "ir.h"
#include <stdio.h>

int cgen_emit_x86_64(FILE *out, ir_program_t *prog);

#endif

