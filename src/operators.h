#pragma once

#ifndef OPERATORS
#define OPERATORS

enum binary_operator {
	operator_equals,
	operator_not_equal,
	operator_more,
	operator_less,
	operator_more_equal,
	operator_less_equal,
	operator_and,
	operator_or,
	operator_add,
	operator_subtract,
	operator_multiply,
	operator_divide,
	operator_modulo,
	operator_power,
};

enum unary_operator {
	operator_copy,
	operator_invert,
	operator_negate,
	operator_alloc,
	operator_increment,
	operator_decriment
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