#pragma once

#ifndef VALUE_H
#define VALUE_H

#include "object.h"

#define IS_COLLECTION(VALUE) ((VALUE).type == VALUE_TYPE_OBJ && (VALUE).payload.object.type == OBJ_TYPE_COL)
#define IS_RECORD(VALUE) ((VALUE).type == VALUE_TYPE_OBJ && (VALUE).payload.object.type == OBJ_TYPE_REC)

#define NUM_VALUE(NUM) (struct value) { .gc_flag = GARBAGE_CONSTANT, .payload.numerical = NUM, .type = VALUE_TYPE_NUM}
#define CHAR_VALUE(CHAR) (struct value) { .gc_flag = GARBAGE_CONSTANT, .payload.character = CHAR, .type = VALUE_TYPE_CHAR}
#define ID_VALUE(IDENTIFIER) (struct value) {.gc_flag = GARBAGE_CONSTANT, .payload.identifier = IDENTIFIER, .type = VALUE_TYPE_ID}

struct value {
	enum garbage_flag {
		GARBAGE_CONSTANT,
		GARBAGE_COLLECT,
		GARBAGE_KEEP
	} gc_flag;

	union value_payload
	{
		double numerical;
		char character;
		uint64_t identifier;
		struct object object;
	} payload;

	enum value_type {
		VALUE_TYPE_NULL,
		VALUE_TYPE_NUM,
		VALUE_TYPE_CHAR,
		VALUE_TYPE_ID,
		VALUE_TYPE_OBJ,
	} type;
};

static const struct value const_value_null = { GARBAGE_CONSTANT, 0, VALUE_TYPE_NULL };
static const struct value const_value_true = { GARBAGE_CONSTANT, 1, VALUE_TYPE_NUM };
static const struct value const_value_false = { GARBAGE_CONSTANT, 0, VALUE_TYPE_NUM };

void init_obj_value(struct value* value, struct object obj);

const double compare_value(const struct value*a, const struct value*b);

void free_value(struct value* value);

#endif // !VALUE_H
