#ifndef STATMENTS_H
#define STATMENTS_H

#include "compiler.h"

const int compile_value(struct compiler* compiler, struct chunk_builder* builder, const int expr_optimize);
const int compile_statement(struct compiler* compiler, struct chunk_builder* builder, const uint64_t callee, const int encapsulated, uint64_t proc_encapsulated);

#endif // !STATMENTS_H