#include <stdlib.h>
#include <string.h>
#include "include/runtime/value.h"

void init_obj_value(struct value* value, struct object obj) {
	value->gc_flag = GARBAGE_UNINIT;
	value->type = VALUE_TYPE_OBJ;
	value->payload.object = obj;
}

const double compare_value(const struct value* a, const struct value* b) {
	if (a->type != b->type)
		return (double)a->type - (double)b->type;

	switch (a->type)
	{
	case VALUE_TYPE_NUM:
		return a->payload.numerical - b->payload.numerical;
	case VALUE_TYPE_CHAR:
		return (double)a->payload.character - (double)b->payload.character;
	case VALUE_TYPE_OBJ:
		return (double)(!object_compare(&a->payload.object, &b->payload.object));
	}
	return 0;
}

void free_value(struct value* value) {
	if (value->type == VALUE_TYPE_OBJ)
		free_object(&value->payload.object);
}