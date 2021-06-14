#pragma once

#ifndef VALUE_H
#define VALUE_H

#include "object.h"

struct value {
	enum garbage_flag {
		garbage_uninit,
		garbage_collect,
		garbage_trace,
		garbage_keep
	}gc_flag;

	union value_payload
	{
		double numerical;
		char character;
		struct object object;
	}payload;

	enum value_type {
		value_type_null,
		value_type_numerical,
		value_type_character,
		value_type_object,
	} type;
};

void init_null_value(struct value* value);
void init_num_value(struct value* value, const double d);
void init_char_value(struct value* value, const char c);
void init_obj_value(struct value* value, struct object obj);

const int copy_value(struct value* dest, struct value* src);
const int compare_value(struct value* a, struct value* b);

void free_value(struct value* value);

#endif // !VALUE_H
