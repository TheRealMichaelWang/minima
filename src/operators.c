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
	struct value c;
	init_num_value(&c, !compare_value(a, b));
	return c;
}

DECL_BINARY_OPERATOR(op_not_equals) {
	struct value c;
	init_num_value(&c, compare_value(a, b));
	return c;
}

DECL_BINARY_OPERATOR(op_more) {
	struct value c;
	init_num_value(&c, compare_value(a, b) > 0);
	return c;
}

DECL_BINARY_OPERATOR(op_less) {
	struct value c;
	init_num_value(&c, compare_value(a, b) < 0);
	return c;
}

DECL_BINARY_OPERATOR(op_more_equal) {
	struct value c;
	init_num_value(&c, compare_value(a, b) >= 0);
	return c;
}

DECL_BINARY_OPERATOR(op_less_equal) {
	struct value c;
	init_num_value(&c, compare_value(a, b) <= 0);
	return c;
}

DECL_BINARY_OPERATOR(op_and) {
	struct value c;
	if (a->type == VALUE_TYPE_NULL || b->type == VALUE_TYPE_NULL)
		init_num_value(&c, 0);
	else if ((a->type == VALUE_TYPE_NUM && a->payload.numerical == 0) ||
		(b->type == VALUE_TYPE_NUM && b->payload.numerical == 0)) {
		init_num_value(&c, 0);
	}
	else
		init_num_value(&c, 1);
	return c;
}

DECL_BINARY_OPERATOR(op_or) {
	struct value c;
	if ((a->type == VALUE_TYPE_NUM && a->payload.numerical == 1) ||
		(b->type == VALUE_TYPE_NUM && b->payload.numerical == 1)) {
		init_num_value(&c, 1);
	}
	else
		init_num_value(&c, 0);
	return c;
}

DECL_BINARY_OPERATOR(op_add) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	//MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	struct value c;
	init_num_value(&c, a->payload.numerical + b->payload.numerical);
	return c;
}

DECL_BINARY_OPERATOR(op_subtract) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	//MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	struct value c;
	double a_num = a->payload.numerical;
	double b_num = b->payload.numerical;
	init_num_value(&c, a_num - b_num);
	return c;
}

DECL_BINARY_OPERATOR(op_multiply) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	//MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	struct value c;
	double a_num = a->payload.numerical;
	double b_num = b->payload.numerical;
	init_num_value(&c, a_num * b_num);
	return c;
}

DECL_BINARY_OPERATOR(op_divide) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	//MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	struct value c;
	double a_num = a->payload.numerical;
	double b_num = b->payload.numerical;
	init_num_value(&c, a_num / b_num);
	return c;
}

DECL_BINARY_OPERATOR(op_modulo) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	//MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	struct value c;
	double a_num = a->payload.numerical;
	double b_num = b->payload.numerical;
	init_num_value(&c, fmod(a_num, b_num));
	return c;
}

DECL_BINARY_OPERATOR(op_power) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	//MATCH_OP_TYPE(b, VALUE_TYPE_NUM);
	struct value c;
	double a_num = a->payload.numerical;
	double b_num = b->payload.numerical;
	init_num_value(&c, powf(a_num, b_num));
	return c;
}

DECL_UNARY_OPERATOR(op_copy) {
	struct value copy = *a;
	copy.gc_flag = GARBAGE_UNINIT;
	return copy;
}

DECL_UNARY_OPERATOR(op_invert) {
	struct value c;
	if (a->type == VALUE_TYPE_NULL || (a->type == VALUE_TYPE_NUM && a->payload.numerical == 0))
		init_num_value(&c, 1);
	init_num_value(&c, 0);
	return c;
}

DECL_UNARY_OPERATOR(op_negate) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	struct value c;
	init_num_value(&c, -a->payload.numerical);
	return c;
}

DECL_UNARY_OPERATOR(op_alloc) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	struct value c;

	uint64_t i = a->payload.numerical;
	if (i > 1000000)
		return const_value_null;
	
	struct collection* collection = malloc(sizeof(struct collection));
	//ERROR_ALLOC_CHECK(collection);

	init_collection(collection, i);
	while (i--) {
		struct value elem = const_value_null;
		collection->inner_collection[i] = push_eval(machine, &elem, 0);
	}
	struct object obj;
	init_object_col(&obj, collection);
	init_obj_value(&c, obj);
	return c;
}

DECL_UNARY_OPERATOR(op_increment) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	struct value c;
	init_num_value(&c, a->payload.numerical);
	a->payload.numerical++;
	return c;
}

DECL_UNARY_OPERATOR(op_decriment) {
	//MATCH_OP_TYPE(a, VALUE_TYPE_NUM);
	struct value c;
	init_num_value(&c, a->payload.numerical);
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