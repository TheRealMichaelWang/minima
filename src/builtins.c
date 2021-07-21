#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
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

struct value str_to_value(const char* buffer, const size_t length, struct machine* machine) {
	struct value toret;
	struct collection* collection = malloc(sizeof(struct collection));
	if (collection == NULL)
		return const_value_null;

	init_collection(collection, length);
	struct value char_elem;
	for(uint_fast32_t i = 0; i < length; i++) {
		char_elem = CHAR_VALUE(buffer[i]);
		collection->inner_collection[i] = machine_push_eval(machine, &char_elem, 0);
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
	if (argc == 1)
		return NUM_VALUE(value_hash(*argv[0]));
	else if (argc == 2)
		return NUM_VALUE(combine_hash(argv[0]->payload.numerical, argv[1]->payload.numerical));
	return const_value_null;
}

DECL_BUILT_IN(builtin_to_num) {
	if (argc < 1)
		return const_value_null;

	char* buffer = value_to_str(*argv[0]);
	if (buffer == NULL)
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

DECL_BUILT_IN(builtin_get_type) {
	if (argc < 1)
		return const_value_null;
	if (!IS_RECORD(*argv[0]) && argv[0]->type != VALUE_TYPE_ID)
		return CHAR_VALUE(argv[0]->type);
	uint64_t type_hash = combine_hash(VALUE_TYPE_OBJ, argv[0]->type == VALUE_TYPE_ID ? argv[0]->payload.identifier : argv[0]->payload.object.ptr.record->prototype->identifier);
	return NUM_VALUE(type_hash);
}

DECL_BUILT_IN(builtin_implements) {
	if (argc < 2 || !IS_RECORD(*argv[0]) || argv[1]->type != VALUE_TYPE_ID)
		return const_value_null;
	struct value* record = argv[0];
	while (record != NULL)
	{
		if (record->payload.object.ptr.record->prototype->identifier == argv[1]->payload.identifier)
			return const_value_true;
		record = record_get_property(record->payload.object.ptr.record, RECORD_BASE_PROPERTY);
	}
	return const_value_false;
}

DECL_BUILT_IN(builtin_abs) {
	if (argc < 1 || argv[0]->type != VALUE_TYPE_NUM)
		return const_value_null;
	if (argv[0]->payload.numerical >= 0)
		return NUM_VALUE(argv[0]->payload.numerical);
	else
		return NUM_VALUE(-argv[0]->payload.numerical);
}

DECL_BUILT_IN(builtin_max) {
	if (argc < 2)
		return const_value_null;
	return NUM_VALUE(max(argv[0]->payload.numerical, argv[1]->payload.numerical));
}

DECL_BUILT_IN(builtin_min) {
	if (argc < 2)
		return const_value_null;
	return NUM_VALUE(min(argv[0]->payload.numerical, argv[1]->payload.numerical));
}

DECL_BUILT_IN(builtin_ceil) {
	if (argc < 1)
		return const_value_null;
	return NUM_VALUE(ceil(argv[0]->payload.numerical));
}

DECL_BUILT_IN(builtin_floor) {
	if (argc < 1)
		return const_value_null;
	return NUM_VALUE(floor(argv[0]->payload.numerical));
}