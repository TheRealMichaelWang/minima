#include <stdlib.h>
#include <math.h>
#include "value.h"
#include "operators.h"

struct value* equals(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, !compare_value(a, b));
	return c;
}

struct value* not_equals(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, compare_value(a, b));
	return c;
}

struct value* more(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, compare_value(a, b) > 0);
	return c;
}

struct value* less(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, compare_value(a, b) < 0);
	return c;
}

struct value* more_equal(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, compare_value(a, b) >= 0);
	return c;
}

struct value* less_equal(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, compare_value(a, b) <= 0);
	return c;
}

struct value* add(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	double a_num = as_double(a);
	double b_num = as_double(b);
	init_num(c, a_num + b_num);
	return c;
}

struct value* subtract(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	double a_num = as_double(a);
	double b_num = as_double(b);
	init_num(c, a_num - b_num);
	return c;
}

struct value* multiply(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	double a_num = as_double(a);
	double b_num = as_double(b);
	init_num(c, a_num * b_num);
	return c;
}

struct value* divide(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	double a_num = as_double(a);
	double b_num = as_double(b);
	init_num(c, a_num / b_num);
	return c;
}

struct value* modulo(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	double a_num = as_double(a);
	double b_num = as_double(b);
	init_num(c, fmod(a_num, b_num));
	return c;
}