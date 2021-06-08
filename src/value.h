#pragma once

#ifndef VALUE_H
#define VALUE_H

struct value {
	enum garbage_flag {
		garbage_uninit,
		garbage_collect,
		garbage_keep,
		garbage_protected
	}gc_flag;

	union value_payload
	{
		double numerical;
		char character;
		struct collection* collection;
	}payload;

	enum value_type {
		null,
		numerical,
		character,
		collection
	} type;
};

struct collection {
	struct value** inner_collection;
	unsigned long size;
};

//value methods

void init_null(struct value* value);
void init_num(struct value* value, const double d);
void init_char(struct value* value, const char c);
void init_col(struct value* value, struct collection* col);

const int copy_value(struct value* dest, struct value* src);
const int compare_value(struct value* a, struct value* b);

void free_value(struct value* value);

//collection methods
int init_collection(struct collection* collection, unsigned long size);
void free_collection(struct collection* collection);

const int copy_collection(struct collection* dest, struct collection* src);
const int compare_collection(struct collection* a, struct collection* b);

#endif // !VALUE_H
