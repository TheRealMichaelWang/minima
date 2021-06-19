#include <stdlib.h>
#include <stdio.h>
#include "collection.h"
#include "record.h"
#include "io.h"

static void print_data_char(const char data_char) {
	switch (data_char)
	{
	case '\t':
		printf("\\t");
		break;
	case '\r':
		printf("\\r");
		break;
	case '\n':
		printf("\\n");
		break;
	case '\b':
		printf("\\b");
		break;
	case 0:
		printf("\\0");
		break;
	case '\\':
		printf("\\");
		break;
	case '\'':
		printf("\\\'");
		break;
	case '\"':
		printf("\\\"");
		break;
	default:
		printf("%c", data_char);
	}
}

static const int print_str(struct collection* str, const int print_mode) {
	for (unsigned long i = 0; i < str->size; i++) {
		if (str->inner_collection[i]->type != VALUE_TYPE_CHAR)
			return 0;
		if (print_mode)
			printf("%c", str->inner_collection[i]->payload.character);
		else
			print_data_char(str->inner_collection[i]->payload.character);
	}
	return 1;
}

static const int is_str(struct value* value) {
	if (IS_COLLECTION(value))
		return 0;
	struct collection* collection = value->payload.object.ptr.collection;
	for (unsigned long i = 0; i < collection->size; i++)
		if (collection->inner_collection[i]->type != VALUE_TYPE_CHAR)
			return 0;
	return 1;
}

static void print_collection(struct collection* collection) {
	printf("%c", '[');
	for (unsigned long i = 0; i < collection->size; i++) {
		if(i)
			printf(", ");
		print_value(collection->inner_collection[i], 0);
	}
	printf("%c", ']');
}

static void print_record(struct record* record) {
	printf("<%p>", record);
	for (unsigned char i = 0; i < record->prototype->size; i++) {
		printf("\n\t");
		print_value(record->properties[i], 0);
	}
}

void print_value(struct value* value, const int print_mode) {
	if (value->type == VALUE_TYPE_NUM)
		printf("%lf", value->payload.numerical);
	else if (value->type == VALUE_TYPE_CHAR)
		if (print_mode)
			printf("%c", value->payload.character);
		else {
			printf("\'");
			print_data_char(value->payload.character);
			printf("\'");
		}
	else if (value->type == VALUE_TYPE_NULL)
		printf("null");
	else if (IS_COLLECTION(value))
		if (is_str(value))
			print_str(value->payload.object.ptr.collection, print_mode);
		else
			print_collection(value->payload.object.ptr.collection);
	else if (IS_RECORD(value))
		if (print_mode)
			print_record(value->payload.object.ptr.record);
		else
			printf("<%p>", value->payload.object.ptr.record);
	else
		printf("[Print Error]");
}