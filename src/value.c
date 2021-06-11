#include <stdlib.h>
#include <string.h>
#include "value.h"

void init_null_value(struct value* value) {
	value->gc_flag = garbage_uninit;
	value->type = value_type_null;
}

void init_num_value(struct value* value, const double num) {
	value->gc_flag = garbage_uninit;
	value->type = value_type_numerical;
	value->payload.numerical = num;
}

void init_char_value(struct value* value, const char c) {
	value->gc_flag = garbage_uninit;
	value->type = value_type_character;
	value->payload.character = c;
}

void init_obj_value(struct value* value, struct object obj) {
	value->gc_flag = garbage_uninit;
	value->type = value_type_object;
	value->payload.object = obj;
}

const int copy_value(struct value* dest, struct value* src) {
	dest->type = src->type;
	memcpy(dest, src, sizeof(struct value));
	dest->gc_flag = garbage_uninit;

	return dest->type != value_type_object;
}

const int compare_value(struct value* a, struct value* b) {
	if (a->type != b->type)
		return a->type - b->type;

	switch (a->type)
	{
	case value_type_null:
		return 1;
	case value_type_numerical: {
		return a->payload.numerical - b->payload.numerical;
	}
	case value_type_character:
		return a->payload.character - b->payload.character;
	case value_type_object: {
		return !compare_object(a, b);
	}
	}
	return 0;
}

void free_value(struct value* value) {
	if (value->type == value_type_object)
		free_object(&value->payload.object);
}