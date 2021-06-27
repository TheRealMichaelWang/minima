#include <stdlib.h>
#include <stdio.h>
#include "object.h"
#include "collection.h"
#include "io.h"
#include "hash.h"
#include "error.h"
#include "builtins.h"

DECL_BUILT_IN(builtin_print) {
	for (uint64_t i = 0; i < argc; i++) {
		print_value(argv[i], 1);
	}
	struct value* nullvalue = malloc(sizeof(struct value));
	ERROR_ALLOC_CHECK(nullvalue);
	init_null_value(nullvalue);
	return nullvalue;
}

DECL_BUILT_IN(builtin_print_line) {
	struct value* toreturn = builtin_print(argv, argc);
	printf("\n");
	return toreturn;
}

DECL_BUILT_IN(builtin_system_cmd) {
	if (argc < 1 || !IS_COLLECTION(argv[0]))
		return NULL;
	struct collection* collection = argv[0]->payload.object.ptr.collection;
	char* buffer = malloc((collection->size + 1)* sizeof(char));
	ERROR_ALLOC_CHECK(buffer);
	for (uint64_t i = 0; i < collection->size; i++) {
		buffer[i] = collection->inner_collection[i]->payload.character;
	}
	buffer[collection->size] = 0;
	system(buffer);
	free(buffer);
	struct value* nullvalue = malloc(sizeof(struct value));
	ERROR_ALLOC_CHECK(nullvalue);
	init_null_value(nullvalue);
	return nullvalue;
}

DECL_BUILT_IN(builtin_random) {
	double random_double = (double)rand() / RAND_MAX;
	struct value* numvalue = malloc(sizeof(struct value));
	ERROR_ALLOC_CHECK(numvalue);
	init_num_value(numvalue,random_double);
	return numvalue;
}

DECL_BUILT_IN(builtin_get_input) {
	char format_flag = 0;
	if (argc > 0 && argv[0]->type == VALUE_TYPE_CHAR)
		format_flag = argv[0]->payload.character;

	char buffer[4096];
	uint32_t length = 0;
	while (scanf_s("%c", &buffer[length], 1)) {
		if (buffer[length] == '\n')
			break;
		length++;
	}
	buffer[length] = 0;

	struct value* toret = malloc(sizeof(struct value));
	ERROR_ALLOC_CHECK(toret);
	if (format_flag == 'n' || format_flag == 'N') {
		init_num_value(toret, strtod(buffer, NULL));
	}
	else {
		struct collection* collection = malloc(sizeof(struct collection));
		ERROR_ALLOC_CHECK(collection);
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

DECL_BUILT_IN(builtin_get_length) {
	if (argc < 1 || !IS_COLLECTION(argv[0]))
		return NULL;
	struct value* toret = malloc(sizeof(struct value));
	ERROR_ALLOC_CHECK(toret);
	init_num_value(toret, argv[0]->payload.object.ptr.collection->size);
	return toret;
}

DECL_BUILT_IN(builtin_get_hash) {
	if (argc < 1)
		return NULL;
	struct value* toret = malloc(sizeof(struct value));
	ERROR_ALLOC_CHECK(toret);
	init_num_value(toret, value_hash(*argv[0]));
	return toret;
}