#include <stdlib.h>
#include <math.h>
#include "include/error.h"
#include "include/runtime/value.h"
#include "include/runtime/machine.h"
#include "include/runtime/operators.h"

#define DECL_BINARY_OPERATOR(METHOD_NAME) static struct value METHOD_NAME(struct value* a, struct value* b)
#define DECL_UNARY_OPERATOR(METHOD_NAME) static struct value METHOD_NAME(struct value* a, struct machine* machine)

//#define MATCH_OP_TYPE(OPERAND, VALUE_TYPE) if(OPERAND->type != VALUE_TYPE) { return 0; } 

DECL_BINARY_OPERATOR(op_equals) {
	return NUM_VALUE(!compare_value(a, b));
}

DECL_BINARY_OPERATOR(op_not_equals) {
	return NUM_VALUE(compare_value(a, b));
}

DECL_BINARY_OPERATOR(op_more) {
	return NUM_VALUE(compare_value(a, b) > 0);
}

DECL_BINARY_OPERATOR(op_less) {
	return NUM_VALUE(compare_value(a, b) < 0);
}

DECL_BINARY_OPERATOR(op_more_equal) {
	return NUM_VALUE(compare_value(a, b) >= 0);
}

DECL_BINARY_OPERATOR(op_less_equal) {
	return NUM_VALUE(compare_value(a, b) <= 0);
}

DECL_BINARY_OPERATOR(op_and) {
	if (a->type == VALUE_TYPE_NULL || b->type == VALUE_TYPE_NULL)
		return const_value_false;
	else if ((a->type == VALUE_TYPE_NUM && a->payload.numerical == 0) ||
		(b->type == VALUE_TYPE_NUM && b->payload.numerical == 0))
		return const_value_false;
	else
		return const_value_true;
}

DECL_BINARY_OPERATOR(op_or) {
	if ((a->type == VALUE_TYPE_NUM && a->payload.numerical == 1) ||
		(b->type == VALUE_TYPE_NUM && b->payload.numerical == 1))
		return const_value_true;
	else
		return const_value_false;
}

DECL_BINARY_OPERATOR(op_add) {
	return NUM_VALUE(a->payload.numerical + b->payload.numerical);
}

DECL_BINARY_OPERATOR(op_subtract) {
	return NUM_VALUE(a->payload.numerical - b->payload.numerical);
}

DECL_BINARY_OPERATOR(op_multiply) {
	return NUM_VALUE(a->payload.numerical * b->payload.numerical);
}

DECL_BINARY_OPERATOR(op_divide) {
	return NUM_VALUE(a->payload.numerical / b->payload.numerical);
}

DECL_BINARY_OPERATOR(op_modulo) {
	return NUM_VALUE(fmod(a->payload.numerical, b->payload.numerical));
}

DECL_BINARY_OPERATOR(op_power) {
	return NUM_VALUE(powf(a->payload.numerical, b->payload.numerical));
}

DECL_UNARY_OPERATOR(op_copy) {
	struct value copy = *a;
	copy.gc_flag = GARBAGE_UNINIT;
	return copy;
}

DECL_UNARY_OPERATOR(op_invert) {
	if (a->type == VALUE_TYPE_NULL || (a->type == VALUE_TYPE_NUM && a->payload.numerical == 0))
		return const_value_true;
	return const_value_false;
}

DECL_UNARY_OPERATOR(op_negate) {
	return NUM_VALUE(-a->payload.numerical);
}

DECL_UNARY_OPERATOR(op_alloc) {
	struct value c;

	uint64_t i = a->payload.numerical;
	if (i > 1000000)
		return const_value_null;
	
	struct collection* collection = malloc(sizeof(struct collection));
	if (collection == NULL)
		return const_value_null;

	init_collection(collection, i);
	while (i--)
		collection->inner_collection[i] = push_eval(machine, &const_value_null, 0);

	struct object obj;
	init_object_col(&obj, collection);
	init_obj_value(&c, obj);
	
	return c;
}

DECL_UNARY_OPERATOR(op_increment) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	struct value c = NUM_VALUE(a->payload.numerical);
	a->payload.numerical++;
	return c;
}

DECL_UNARY_OPERATOR(op_decriment) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	struct value c = NUM_VALUE(a->payload.numerical);
	a->payload.numerical--;
	return c;
}

DECL_BINARY_OPERATOR((*binary_operators[14])) = {
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

DECL_UNARY_OPERATOR((*unary_operators[6])) = {
	op_copy,
	op_invert,
	op_negate,
	op_alloc,
	op_increment,
	op_decriment
};

struct value invoke_binary_op(enum binary_operator operator, struct value* a, struct value* b) {
	return (*binary_operators[operator])(a, b);
}

struct value invoke_unary_op(enum unary_operator operator, struct value* a, struct machine* machine) {
	return (*unary_operators[operator])(a, machine);
}