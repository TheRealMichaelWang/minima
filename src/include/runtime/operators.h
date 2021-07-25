#pragma once

#ifndef OPERATORS_H
#define OPERATORS_H

#include "value.h"

enum binary_operator {
	OPERATOR_EQUALS,
	OPERATOR_NOT_EQUAL,
	OPERATOR_MORE,
	OPERATOR_LESS,
	OPERATOR_MORE_EQUAL,
	OPERATOR_LESS_EQUAL,
	OPERATOR_AND,
	OPERATOR_OR,
	OPERATOR_ADD,
	OPERATOR_SUBTRACT,
	OPERATOR_MULTIPLY,
	OPERATOR_DIVIDE,
	OPERATOR_MODULO,
	OPERATOR_POWER
};

enum unary_operator {
	OPERATOR_COPY,
	OPERATOR_INVERT,
	OPERATOR_NEGATE,
	OPERATOR_ALLOC,
	OPERATOR_INCREMENT,
	OPERATOR_DECRIMENT
};

enum op_precedence {
	PREC_BEGIN,
	PREC_LOGIC,
	PREC_COMP,
	PREC_ADD,
	PREC_MULTIPLY,
	PREC_EXP,
};

static enum op_precedence op_precedence[14] = {
	PREC_COMP,
	PREC_COMP,
	PREC_COMP,
	PREC_COMP,
	PREC_COMP,
	PREC_COMP,
	PREC_LOGIC,
	PREC_LOGIC,
	PREC_ADD,
	PREC_ADD,
	PREC_MULTIPLY,
	PREC_MULTIPLY,
	PREC_MULTIPLY,
	PREC_EXP
};

struct value* invoke_binary_op(enum binary_operator operator, struct value* a, struct value* b, struct machine* machine);
struct value* invoke_unary_op(enum unary_operator operator, struct value* a, struct machine* machine);

#endif // !OPERATORS