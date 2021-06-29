#pragma once

#ifndef VALUE_H
#define VALUE_H

#include "object/object.h"

#define IS_COLLECTION(VALUE) (VALUE->type == VALUE_TYPE_OBJ && VALUE->payload.object.type == OBJ_TYPE_COL)
#define IS_RECORD(VALUE) (VALUE->type == VALUE_TYPE_OBJ && VALUE->payload.object.type == OBJ_TYPE_REC)

struct value {
	enum garbage_flag {
		GARBAGE_UNINIT,
		GARBAGE_COLLECT,
		GARBAGE_TRACE,
		GARBAGE_KEEP
	}gc_flag;

	union value_payload
	{
		double numerical;
		char character;
		struct object object;
	}payload;

	enum value_type {
		VALUE_TYPE_NULL,
		VALUE_TYPE_NUM,
		VALUE_TYPE_CHAR,
		VALUE_TYPE_OBJ,
	} type;
};

static const struct value const_value_null = { GARBAGE_UNINIT, 0, VALUE_TYPE_NULL };
static const struct value const_value_true = { GARBAGE_UNINIT, 1, VALUE_TYPE_NUM };
static const struct value const_value_false = { GARBAGE_UNINIT, 0, VALUE_TYPE_NUM };

void init_null_value(struct value* value);
void init_num_value(struct value* value, const double d);
void init_char_value(struct value* value, const char c);
void init_obj_value(struct value* value, struct object obj);

const int copy_value(struct value* dest, struct value* src);
const double compare_value(const struct value* a, const struct value* b);

void free_value(struct value* value);

#endif // !VALUE_H
