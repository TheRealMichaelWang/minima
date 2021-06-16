#include <stdlib.h>
#include <stdio.h>
#include "object.h"
#include "collection.h"
#include "io.h"
#include "stdlib.h"

struct value* builtin_print(struct value** argv, unsigned int argc) {
	for (unsigned long i = 0; i < argc; i++) {
		print_value(argv[i], 1);
	}
	struct value* nullvalue = malloc(sizeof(struct value));
	init_null_value(nullvalue);
	return nullvalue;
}

struct value* builtin_print_line(struct value** argv, unsigned int argc) {
	struct value* toreturn = builtin_print(argv, argc);
	printf("\n");
	return toreturn;
}

struct value* builtin_system_cmd(struct value** argv, unsigned int argc) {
	if (argc < 1 || argv[0]->type != value_type_object || argv[0]->payload.object.type != obj_type_collection)
		return NULL;
	struct collection* collection = argv[0]->payload.object.ptr.collection;
	char* buffer = malloc((collection->size + 1)* sizeof(char));
	for (unsigned long i = 0; i < collection->size; i++) {
		buffer[i] = collection->inner_collection[i]->payload.character;
	}
	buffer[collection->size] = 0;
	system(buffer);
	free(buffer);
	struct value* nullvalue = malloc(sizeof(struct value));
	init_null_value(nullvalue);
	return nullvalue;
}

struct value* builtin_random(struct value** argv, unsigned int argc) {
	double random_double = (double)rand() / RAND_MAX;
	struct value* numvalue = malloc(sizeof(struct value));
	init_num_value(numvalue,random_double);
	return numvalue;
}

struct value* builtin_get_input(struct value** argv, unsigned int argc) {
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

struct value* builtin_get_length(struct value** argv, unsigned int argc) {
	if (argc < 1 || argv[0]->type != value_type_object || argv[0]->payload.object.type != obj_type_collection)
		return NULL;
	struct value* toret = malloc(sizeof(struct value));
	init_num_value(toret, argv[0]->payload.object.ptr.collection->size);
	return toret;
}