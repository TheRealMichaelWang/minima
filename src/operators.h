#pragma once

#ifndef OPERATORS
#define OPERATORS

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

struct value* op_equals(struct value* a, struct value* b);
struct value* op_not_equals(struct value* a, struct value* b);
struct value* op_more(struct value* a, struct value* b);
struct value* op_less(struct value* a, struct value* b);
struct value* op_more_equal(struct value* a, struct value* b);
struct value* op_less_equal(struct value* a, struct value* b);

struct value* op_and(struct value* a, struct value* b);
struct value* op_or(struct value* a, struct value* b);

struct value* op_add(struct value* a, struct value* b);
struct value* op_subtract(struct value* a, struct value* b);
struct value* op_multiply(struct value* a, struct value* b);
struct value* op_divide(struct value* a, struct value* b);
struct value* op_modulo(struct value* a, struct value* b);
struct value* op_power(struct value* a, struct value* b);

struct value* op_copy(struct value* a);

struct value* op_invert(struct value* a);
struct value* op_negate(struct value* a);

struct value* op_alloc(struct value* a);

struct value* op_increment(struct value* a);

struct value* op_decriment(struct value* a);

#endif // !OPERATORS