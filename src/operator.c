#include <stdlib.h>
#include <math.h>
#include "value.h"
#include "operators.h"

struct value* op_equals(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, !compare_value(a, b));
	return c;
}

struct value* op_not_equals(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, compare_value(a, b));
	return c;
}

struct value* op_more(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, compare_value(a, b) > 0);
	return c;
}

struct value* op_less(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, compare_value(a, b) < 0);
	return c;
}

struct value* op_more_equal(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, compare_value(a, b) >= 0);
	return c;
}

struct value* op_less_equal(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, compare_value(a, b) <= 0);
	return c;
}

struct value* op_and(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	if (a->type == null || b->type == null)
		init_num(c, 0);
	else if ((a->type == numerical && a->payload.numerical == 0) ||
		(b->type == numerical && b->payload.numerical == 0)) {
		init_num(c, 0);
	}
	else
		init_num(c, 1);
	return c;
}

struct value* op_or(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value)); 
	if ((a->type == numerical && a->payload.numerical == 1) ||
		(b->type == numerical && b->payload.numerical == 1)) {
		init_num(c, 1);
	}
	else
		init_num(c, 0);
	return c;
}

struct value* op_add(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, a->payload.numerical + b->payload.numerical);
	return c;
}

struct value* op_subtract(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	double a_num = a->payload.numerical;
	double b_num = b->payload.numerical;
	init_num(c, a_num - b_num);
	return c;
}

struct value* op_multiply(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	double a_num = a->payload.numerical;
	double b_num = b->payload.numerical;
	init_num(c, a_num * b_num);
	return c;
}

struct value* op_divide(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	double a_num = a->payload.numerical;
	double b_num = b->payload.numerical;
	init_num(c, a_num / b_num);
	return c;
}

struct value* op_modulo(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	double a_num = a->payload.numerical;
	double b_num = b->payload.numerical;
	init_num(c, fmod(a_num, b_num));
	return c;
}

struct value* op_power(struct value* a, struct value* b) {
	struct value* c = malloc(sizeof(struct value));
	double a_num = a->payload.numerical;
	double b_num = b->payload.numerical;
	init_num(c, powf(a_num, b_num));
	return c;
}

struct value* op_copy(struct value* a) {
	if (a->type == collection)
		return a;
	struct value* c = malloc(sizeof(struct value));
	copy_value(c, a);
	return c;
}

struct value* op_invert(struct value* a) {
	struct value* c = malloc(sizeof(struct value));
	if (a->type == null || (a->type == numerical && a->payload.numerical == 0))
		init_num(c, 1);
	init_num(c, 0);
	return c;
}

struct value* op_negate(struct value* a) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, -a->payload.numerical);
	return c;
}

struct value* op_alloc(struct value* a) {
	struct value* c = malloc(sizeof(struct value));
	unsigned long i = a->payload.numerical;
	if (i > 1000000) {
		init_null(c);
		return c;
	}
	struct collection* collection = malloc(sizeof(struct collection));
	init_collection(collection, i);
	while (i--)
	{
		collection->inner_collection[i] = malloc(sizeof(struct value));
		init_null(collection->inner_collection[i]);
	}
	init_col(c, collection);
	return c;
}

struct value* op_increment(struct value* a) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, a->payload.numerical);
	a->payload.numerical++;
	return c;
}

struct value* op_decriment(struct value* a) {
	struct value* c = malloc(sizeof(struct value));
	init_num(c, a->payload.numerical);
	a->payload.numerical--;
	return c;
}