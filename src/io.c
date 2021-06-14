#include <stdlib.h>
#include <stdio.h>
#include "collection.h"
#include "record.h"
#include "io.h"

void print_data_char(const char data_char) {
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

const int print_str(struct collection* str, const int print_mode) {
	for (unsigned long i = 0; i < str->size; i++) {
		if (str->inner_collection[i]->type != value_type_character)
			return 0;
		if (print_mode)
			printf("%c", str->inner_collection[i]->payload.character);
		else
			print_data_char(str->inner_collection[i]->payload.character);
	}
	return 1;
}

const int is_str(struct value* value) {
	if (value->type != value_type_object || value->payload.object.type != obj_type_collection)
		return 0;
	struct collection* collection = value->payload.object.ptr.collection;
	for (unsigned long i = 0; i < collection->size; i++)
		if (collection->inner_collection[i]->type != value_type_character)
			return 0;
	return 1;
}

void print_collection(struct collection* collection) {
	printf("%c", '[');
	for (unsigned long i = 0; i < collection->size; i++) {
		if(i)
			printf(", ");
		print_value(collection->inner_collection[i], 0);
	}
	printf("%c", ']');
}

void print_record(struct record* record) {
	printf("<%p>", record);
	for (unsigned char i = 0; i < record->prototype->size; i++) {
		printf("\n\t");
		print_value(record->properties[i], 0);
	}
}

void print_value(struct value* value, const int print_mode) {
	if (value->type == value_type_numerical)
		printf("%lf", value->payload.numerical);
	else if (value->type == value_type_character)
		if (print_mode)
			printf("%c", value->payload.character);
		else {
			printf("\'");
			print_data_char(value->payload.character);
			printf("\'");
		}
	else if (value->type == value_type_null)
		printf("null");
	else if (value->type == value_type_object && value->payload.object.type == obj_type_collection)
		if (is_str(value))
			print_str(value->payload.object.ptr.collection, print_mode);
		else
			print_collection(value->payload.object.ptr.collection);
	else if (value->type == value_type_object && value->payload.object.type == obj_type_record)
		if (print_mode)
			print_record(value->payload.object.ptr.record);
		else
			printf("<%p>", value->payload.object.ptr.record);
	else
		printf("[Print Error]");
}

struct value* print(struct value** argv, unsigned int argc) {
	for (unsigned long i = 0; i < argc; i++) {
		print_value(argv[i], 1);
	}
	struct value* nullvalue = malloc(sizeof(struct value));
	init_null_value(nullvalue);
	return nullvalue;
}

struct value* get_input(struct value** argv, unsigned int argc) {
	char format_flag = 0;
	if (argc > 0 && argv[0]->type == value_type_character)
		format_flag = argv[0]->payload.character;
	
	char buffer[4096];
	unsigned int length = 0;
	while (scanf_s("%c", &buffer[length], 1)) {
		if (buffer[length] == '\n')
			break;
		length++;
	}
	buffer[length] = 0;
	length--;
	
	struct value* toret = malloc(sizeof(struct value));
	if (format_flag == 'n' || format_flag == 'N') {
		init_num_value(toret, strtod(buffer, NULL));
	}
	else {
		struct collection* collection = malloc(sizeof(struct collection));
		init_collection(collection, length);
		while (length--) {
			collection->inner_collection[length] = malloc(sizeof(struct value));
			init_char_value(collection->inner_collection[length], buffer[length]);
		}
		struct object obj;
		init_object_col(&obj, collection);
		init_obj_value(toret, obj);
	}
	return toret;
}

struct value* get_length(struct value** argv, unsigned int argc) {
	if (argc < 1 || argv[0]->type != value_type_object || argv[0]->payload.object.type != obj_type_collection)
		return NULL;
	struct value* toret = malloc(sizeof(struct value));
	init_num_value(toret, argv[0]->payload.object.ptr.collection->size);
	return toret;
}