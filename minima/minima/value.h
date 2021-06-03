#pragma once

#ifndef VALUE_H
#define VALUE_H

#define VALUE_TYPE_NULL 0
#define VALUE_TYPE_NUM 1
#define VALUE_TYPE_CHAR 2
#define VALUE_TYPE_COL 3
#define VALUE_TYPE_RECORD 4

struct value {
	char type;
	void* ptr;
	char gc_flag;
};

void init_value(struct value* value, const char type, void* ptr);

void init_num(struct value* value, const double d);
void init_char(struct value* value, const char c);

const int copy_value(struct value* dest, struct value* src);
const int compare_value(struct value* a, struct value* b);

void free_value(struct value* value);

inline double as_double(struct value* value) {
	return *(double*)value->ptr;
}

inline char as_char(struct value* value) {
	return *(char*)value->ptr;
}

#endif // !VALUE_H
