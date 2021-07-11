#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "include/io.h"
#include "include/hash.h"
#include "include/error.h"
#include "include/runtime/machine.h"
#include "include/runtime/object/object.h"
#include "include/runtime/builtins/builtins.h"

static char* value_to_str(struct value value) {
	if (!IS_COLLECTION(value))
		return NULL;

	struct collection* collection = value.payload.object.ptr.collection;

	char* buffer = malloc((collection->size + 1) * sizeof(char));
	ERROR_ALLOC_CHECK(buffer);
	
	for (uint64_t i = 0; i < collection->size; i++)
		buffer[i] = collection->inner_collection[i]->payload.character;
	buffer[collection->size] = 0;

	return buffer;
}

struct value str_to_value(const char* buffer, const int length, struct machine* machine) {
	struct value toret;
	struct collection* collection = malloc(sizeof(struct collection));
	if (collection == NULL)
		return const_value_null;

	init_collection(collection, length);
	struct value char_elem;
	for(uint_fast32_t i = 0; i < length; i++) {
		char_elem = CHAR_VALUE(buffer[i]);
		collection->inner_collection[i] = push_eval(machine, &char_elem, 0);
	}

	struct object obj;
	init_object_col(&obj, collection);
	init_obj_value(&toret, obj);
	return toret;
}

DECL_BUILT_IN(builtin_print) {
	for (uint64_t i = 0; i < argc; i++) {
		print_value(*argv[i], 1);
	}
	return const_value_null;
}

DECL_BUILT_IN(builtin_print_line) {
	builtin_print(argv, argc, machine);
	printf("\n");
	return const_value_null;
}

DECL_BUILT_IN(builtin_system_cmd) {
	if (argc < 1)
		return const_value_null;

	char* buffer = value_to_str(*argv[0]);
	if(buffer == NULL)
		return const_value_null;

	system(buffer);
	free(buffer);

	return const_value_null;
}

DECL_BUILT_IN(builtin_random) {
	double random_double = (double)rand() / RAND_MAX;
	return NUM_VALUE(random_double);
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

	if (format_flag == 'n' || format_flag == 'N')
		return NUM_VALUE(strtod(buffer, NULL));
	else
		return str_to_value(buffer, length, machine);
}

DECL_BUILT_IN(builtin_get_length) {
	if (argc < 1 || !IS_COLLECTION(*argv[0]))
		return const_value_null;
	return NUM_VALUE(argv[0]->payload.object.ptr.collection->size);
}

DECL_BUILT_IN(builtin_get_hash) {
	return NUM_VALUE(value_hash(*argv[0]));
}

DECL_BUILT_IN(builtin_to_num) {
	if (argc < 1)
		return const_value_null;

	char* buffer = value_to_str(*argv[0]);
	if (*buffer == NULL)
		return const_value_null;
	struct value toret = NUM_VALUE(strtod(buffer, NULL));
	free(buffer);
	return toret;
}

DECL_BUILT_IN(builtin_to_str) {
	if (argc < 1)
		return const_value_null;

	if (argv[0]->type != VALUE_TYPE_NUM)
		return const_value_null;

	char* buffer = malloc(150);
	if (buffer == NULL)
		return const_value_null;

	sprintf(buffer, "%g", argv[0]->payload.numerical);
	struct value toret = str_to_value(buffer, strlen(buffer), machine);
	free(buffer);

	return toret;
}