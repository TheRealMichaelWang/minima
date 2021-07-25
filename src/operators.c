#include <stdlib.h>
#include <math.h>
#include "include/error.h"
#include "include/runtime/value.h"
#include "include/runtime/machine.h"
#include "include/runtime/operators.h"

#include <stdio.h>

#define DECL_BINARY_OPERATOR(METHOD_NAME) static struct value* METHOD_NAME(struct value* a, struct value* b, struct machine* machine)
#define DECL_UNARY_OPERATOR(METHOD_NAME) static struct value* METHOD_NAME(struct value* a, struct machine* machine)

#define MATCH_OP_TYPE(OPERAND, VALUE_TYPE) if(OPERAND->type != VALUE_TYPE) { return 0; } 

DECL_BINARY_OPERATOR(op_equals) {
	return machine_push_const(machine, NUM_VALUE(compare_value(a, b) == 0), 0);
}

DECL_BINARY_OPERATOR(op_not_equals) {
	return machine_push_const(machine, NUM_VALUE(compare_value(a, b) != 0), 0);
}

DECL_BINARY_OPERATOR(op_more) {
	return machine_push_const(machine, NUM_VALUE(compare_value(a, b) > 0), 0);
}

DECL_BINARY_OPERATOR(op_less) {
	return machine_push_const(machine, NUM_VALUE(compare_value(a, b) < 0), 0);
}

DECL_BINARY_OPERATOR(op_more_equal) {
	return machine_push_const(machine, NUM_VALUE(compare_value(a, b) >= 0), 0);
}

DECL_BINARY_OPERATOR(op_less_equal) {
	return machine_push_const(machine, NUM_VALUE(compare_value(a, b) <= 0), 0);
}

DECL_BINARY_OPERATOR(op_and) {
	if (a->type == VALUE_TYPE_NULL || b->type == VALUE_TYPE_NULL)
		return machine_push_const(machine, const_value_false, 0);
	else if ((a->type == VALUE_TYPE_NUM && a->payload.numerical == 0) ||
		(b->type == VALUE_TYPE_NUM && b->payload.numerical == 0))
		return machine_push_const(machine, const_value_false, 0);
	else
		return machine_push_const(machine, const_value_true, 0);
}

DECL_BINARY_OPERATOR(op_or) {
	if ((a->type == VALUE_TYPE_NUM && a->payload.numerical == 1) ||
		(b->type == VALUE_TYPE_NUM && b->payload.numerical == 1))
		return machine_push_const(machine, const_value_true, 0);
	else
		return machine_push_const(machine, const_value_false, 0);
}

DECL_BINARY_OPERATOR(op_add) {
	MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	return machine_push_const(machine, NUM_VALUE(a->payload.numerical + b->payload.numerical), 0);
}

DECL_BINARY_OPERATOR(op_subtract) {
	MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	return machine_push_const(machine, NUM_VALUE(a->payload.numerical - b->payload.numerical), 0);
}

DECL_BINARY_OPERATOR(op_multiply) {
	MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	return machine_push_const(machine, NUM_VALUE(a->payload.numerical * b->payload.numerical), 0);
}

DECL_BINARY_OPERATOR(op_divide) {
	MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	return machine_push_const(machine, NUM_VALUE(a->payload.numerical / b->payload.numerical), 0);
}

DECL_BINARY_OPERATOR(op_modulo) {
	MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	MATCH_OP_TYPE(b, VALUE_TYPE_NUM); 
	return machine_push_const(machine, NUM_VALUE(fmod(a->payload.numerical, b->payload.numerical)), 0);
}

DECL_BINARY_OPERATOR(op_power) {
	MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	return machine_push_const(machine, NUM_VALUE(powf(a->payload.numerical, b->payload.numerical)), 0);
}

DECL_UNARY_OPERATOR(op_copy) {
	if (a->type == VALUE_TYPE_OBJ)
		return machine_push_eval(machine, a);
	return machine_push_const(machine, *a, 1);
}

DECL_UNARY_OPERATOR(op_invert) {
	if (a->type == VALUE_TYPE_NULL || (a->type == VALUE_TYPE_NUM && a->payload.numerical == 0))
		return machine_push_const(machine, const_value_true, 0);
	return machine_push_const(machine, const_value_false, 0);
}

DECL_UNARY_OPERATOR(op_negate) {
	return machine_push_const(machine, NUM_VALUE(-a->payload.numerical), 0);
}

DECL_UNARY_OPERATOR(op_alloc) {
	struct value c;

	uint64_t i = a->payload.numerical;
	if (i > 1000000)
		return NULL;
	
	struct collection* collection = malloc(sizeof(struct collection));
	ERROR_ALLOC_CHECK(collection);

	init_collection(collection, i);

	while (i--)
		ERROR_ALLOC_CHECK(collection->inner_collection[i] = machine_push_const(machine, const_value_null, 0));

	struct object obj;
	init_object_col(&obj, collection);
	init_obj_value(&c, obj);
	
	return machine_push_const(machine, c, 0);
}

DECL_UNARY_OPERATOR(op_increment) {
	MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	return machine_push_const(machine, NUM_VALUE(a->payload.numerical++), 0);
}

DECL_UNARY_OPERATOR(op_decriment) {
	MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	return machine_push_const(machine, NUM_VALUE(a->payload.numerical--), 0);
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

struct value* invoke_binary_op(enum binary_operator operator, struct value* a, struct value* b, struct machine* machine) {
	return (*binary_operators[operator])(a, b, machine);
}

struct value* invoke_unary_op(enum unary_operator operator, struct value* a, struct machine* machine) {
	return (*unary_operators[operator])(a, machine);
}