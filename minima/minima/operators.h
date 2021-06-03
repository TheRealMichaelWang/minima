#pragma once

#ifndef OPERATORS
#define OPERATORS

#define OPERATOR_GET_INDEX 0

#define OPERATOR_EQUALS 1
#define OPERATOR_NOT_EQUAL 2
#define OPERATOR_MORE 3
#define OPERATOR_LESS 4
#define OPERATOR_MORE_EQUAL 5
#define OPERATOR_LESS_EQUAL 6

#define OPERATOR_ADD 7
#define OPERATOR_SUBTRACT 8
#define OPERATOR_MULTIPLY 9
#define OPERATOR_DIVIDE 10
#define	OPERATOR_MODULO 11

#define OPERATOR_GET_REF 0

#define OPERATOR_INVERT 1
#define OPERATOR_NEGATE 2

struct value* equals(struct value* a, struct value* b);
struct value* not_equals(struct value* a, struct value* b);
struct value* more(struct value* a, struct value* b);
struct value* less(struct value* a, struct value* b);
struct value* more_equal(struct value* a, struct value* b);
struct value* less_equal(struct value* a, struct value* b);

struct value* add(struct value* a, struct value* b);
struct value* subtract(struct value* a, struct value* b);
struct value* multiply(struct value* a, struct value* b);
struct value* divide(struct value* a, struct value* b);
struct value* modulo(struct value* a, struct value* b);

#endif // !OPERATORS