#include <stdlib.h>
#include <string.h>
#include "include/runtime/value.h"

void init_null_value(struct value* value) {
	value->gc_flag = GARBAGE_UNINIT;
	value->type = VALUE_TYPE_NULL;
}

void init_num_value(struct value* value, const double num) {
	value->gc_flag = GARBAGE_UNINIT;
	value->type = VALUE_TYPE_NUM;
	value->payload.numerical = num;
}

void init_char_value(struct value* value, const char c) {
	value->gc_flag = GARBAGE_UNINIT;
	value->type = VALUE_TYPE_CHAR;
	value->payload.character = c;
}

void init_obj_value(struct value* value, struct object obj) {
	value->gc_flag = GARBAGE_UNINIT;
	value->type = VALUE_TYPE_OBJ;
	value->payload.object = obj;
}

const int copy_value(struct value* dest, struct value* src) {
	dest->type = src->type;
	memcpy(dest, src, sizeof(struct value));
	dest->gc_flag = GARBAGE_UNINIT;

	return dest->type != VALUE_TYPE_OBJ;
}

const double compare_value(const struct value* a, const struct value* b) {
	if (a->type != b->type)
		return (double)a->type - (double)b->type;

	switch (a->type)
	{
	case VALUE_TYPE_NUM: {
		return a->payload.numerical - b->payload.numerical;
	}
	case VALUE_TYPE_CHAR:
		return (double)a->payload.character - (double)b->payload.character;
	case VALUE_TYPE_OBJ: {
		return !object_compare(a, b);
	}
	}
	return 0;
}

void free_value(struct value* value) {
	if (value->type == VALUE_TYPE_OBJ)
		free_object(&value->payload.object, value->gc_flag);
}