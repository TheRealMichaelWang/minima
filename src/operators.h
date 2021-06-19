#pragma once

#ifndef OPERATORS_H
#define OPERATORS_H

#include "value.h"

#define DECL_BINARY_OPERATOR(METHOD_NAME) struct value* METHOD_NAME(struct value* a, struct value* b)
#define DECL_UNARY_OPERATOR(METHOD_NAME) struct value* METHOD_NAME(struct value* a)

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
	OPERATOR_POWER,
};

enum unary_operator {
	OPERATOR_COPY,
	OPERATOR_INVERT,
	OPERATOR_NEGATE,
	OPERATOR_ALLOC,
	OPERATOR_INCREMENT,
	OPERATOR_DECRIMENT
};

DECL_BINARY_OPERATOR(op_equals);
DECL_BINARY_OPERATOR(op_not_equals);
DECL_BINARY_OPERATOR(op_more);
DECL_BINARY_OPERATOR(op_less);
DECL_BINARY_OPERATOR(op_more_equal);
DECL_BINARY_OPERATOR(op_less_equal);

DECL_BINARY_OPERATOR(op_and);
DECL_BINARY_OPERATOR(op_or);

DECL_BINARY_OPERATOR(op_add);
DECL_BINARY_OPERATOR(op_subtract);
DECL_BINARY_OPERATOR(op_multiply);
DECL_BINARY_OPERATOR(op_divide);
DECL_BINARY_OPERATOR(op_modulo);
DECL_BINARY_OPERATOR(op_power);

DECL_UNARY_OPERATOR(op_copy);
DECL_UNARY_OPERATOR(op_invert);
DECL_UNARY_OPERATOR(op_negate);
DECL_UNARY_OPERATOR(op_alloc);
DECL_UNARY_OPERATOR(op_increment);
DECL_UNARY_OPERATOR(op_decriment);

static struct value* (*binary_operators[14])(struct value* a, struct value* b) = {
	op_equals,
	op_not_equals,
	op_more,
	op_less,
	op_more_equal,
	op_less_equal,
	op_and,
	op_or,
	op_add,
	op_subtract,
	op_multiply,
	op_divide,
	op_modulo,
	op_power
};

static struct value* (*unary_operators[6])(struct value* a) = {
	op_copy,
	op_invert,
	op_negate,
	op_alloc,
	op_increment,
	op_decriment
};

#endif // !OPERATORS