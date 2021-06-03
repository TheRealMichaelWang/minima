#include <stdlib.h>
#include <string.h>
#include "value.h"
#include "collection.h"

void init_value(struct value* value, const char type, void* ptr) {
	value->type = type;
	value->ptr = ptr;
}

void init_num(struct value* value, const double d) {
	init_value(value, VALUE_TYPE_NUM, malloc(sizeof(double)));
	memcpy(value->ptr, &d, sizeof(double));
}

void init_char(struct value* value, const char c) {
	init_value(value, VALUE_TYPE_NULL, malloc(sizeof(char)));
	memcpy(value->ptr, &c, sizeof(char));
}

const int copy_value(struct value* dest, struct value* src) {
	dest->type = src->type;
	dest->gc_flag = src->gc_flag;
	switch (src->type)
	{
	case VALUE_TYPE_NULL:
		break;
	case VALUE_TYPE_CHAR:
		dest->ptr = malloc(sizeof(char));
		if (dest->ptr == NULL)
			return 0;
		memcpy(dest->ptr, src->ptr, sizeof(char));
		break;
	case VALUE_TYPE_NUM:
		dest->ptr = malloc(sizeof(double));
		if (dest->ptr == NULL)
			return 0;
		memcpy(dest->ptr, src->ptr, sizeof(double));
		break;
	case VALUE_TYPE_COL:
		dest->ptr = malloc(sizeof(struct collection));
		if (dest->ptr == NULL)
			return 0;
		if (!copy_collection(dest->ptr, src->ptr))
			return 0;
		break;
	default:
		return 0;
	}
	return 1;
}

const int compare_value(struct value* a, struct value* b) {
	if (a->type != b->type)
		return a->type - b->type;
	switch (a->type)
	{
	case VALUE_TYPE_NULL:
		return 1;
	case VALUE_TYPE_NUM: {
		double a_num = as_double(a);
		double b_num = as_double(b);
		return a_num - b_num;
	}
	case VALUE_TYPE_CHAR:
		return memcmp(a->ptr, b->ptr, sizeof(char));
	case VALUE_TYPE_COL: {
		struct collection* a_col = a->ptr;
		struct collection* b_col = b->ptr;
		return !compare_collection(a_col, b_col);
	}
	}
	return 0;
}

void free_value(struct value* value) {
	if (value->type == VALUE_TYPE_COL)
		free_collection(value->ptr);
	free(value->ptr);
}