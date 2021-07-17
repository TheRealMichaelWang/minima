#ifndef STATMENTS_H
#define STATMENTS_H

#include "compiler.h"

const int compile_value(struct compiler* compiler, const int expr_optimize);
const int compile_statement(struct compiler* compiler, const uint64_t callee, const int control_encapsulated, const int proc_encapsulated);

#endif // !STATMENTS_H