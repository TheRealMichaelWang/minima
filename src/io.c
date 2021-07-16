#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "include/error.h"
#include "include/io.h"

static const char* error_names[] = {
	"Program Success",
	"Out of Memory",
	"Insufficient Evaluations",
	"Insufficient Call Size",
	"Unrecognized/Invalid OpCode",

	"Attempted Label Redefine",
	"Undefined Label",
	"Attempted Record-Prototype Redefine",
	"Undefined Record-Prototype",
	"Attempted Record-Property Redefine",
	"Unedfined Record-Property",

	"Unexpected Type",
	"Index out of Range",
	"Stack Overflow",
	"Undefined Variable",
	
	"Unrecognized Token",
	"Unrecognized Control-Sequence",
	"Unexpected Character",
	"Unexpected Token",

	"Cannot Open File"
};

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

static const int print_str(struct collection str, const int print_mode) {
	for (uint_fast64_t i = 0; i < str.size; i++) {
		if (str.inner_collection[i]->type != VALUE_TYPE_CHAR)
			return 0;
		if (print_mode)
			printf("%c", str.inner_collection[i]->payload.character);
		else
			print_data_char(str.inner_collection[i]->payload.character);
	}
	return 1;
}

static const int is_str(struct value value) {
	if (!IS_COLLECTION(value))
		return 0;
	struct collection* collection = value.payload.object.ptr.collection;
	for (uint_fast64_t i = 0; i < collection->size; i++)
		if (collection->inner_collection[i]->type != VALUE_TYPE_CHAR)
			return 0;
	return 1;
}

static void print_collection(struct collection collection) {
	printf("%c", '[');
	for (uint_fast64_t i = 0; i < collection.size; i++) {
		if(i)
			printf(", ");
		print_value(*collection.inner_collection[i], 0);
	}
	printf("%c", ']');
}

static void print_record(struct record record) {
	printf("<%p>", &record);
	for (uint_fast8_t i = 0; i < record.prototype->size; i++) {
		printf("\n\t");
		print_value(*record.properties[i], 0);
	}
}

void print_value(struct value value, const int print_mode) {
	if (value.type == VALUE_TYPE_NUM)
		printf("%g", value.payload.numerical);
	else if (value.type == VALUE_TYPE_CHAR) {
		if (print_mode)
			printf("%c", value.payload.character);
		else {
			printf("\'");
			print_data_char(value.payload.character);
			printf("\'");
		}
	}
	else if (value.type == VALUE_TYPE_NULL)
		printf("null");
	else if (value.type == VALUE_TYPE_ID)
		printf("identifier(%" PRIu64 ")", value.payload.identifier);
	else if (IS_COLLECTION(value))
		if (is_str(value))
			print_str(*value.payload.object.ptr.collection, print_mode);
		else
			print_collection(*value.payload.object.ptr.collection);
	else if (IS_RECORD(value))
		if (print_mode)
			print_record(*value.payload.object.ptr.record);
		else
			printf("<%p>", value.payload.object.ptr.record);
	else
		printf("[Print Error]");
}

const int error_info(enum error error) {
	printf("Error: %s",error_names[error]);
	char* doc_path = malloc(25);
	ERROR_ALLOC_CHECK(doc_path);

	sprintf(doc_path, "docs/error%d.txt", error);
	
	FILE* infile = fopen(doc_path, "rb");
	free(doc_path);

	if (infile) {
		fseek(infile, 0, SEEK_END);
		uint64_t fsize = ftell(infile);
		fseek(infile, 0, SEEK_SET);
		char* source = malloc(fsize + 1);
		ERROR_ALLOC_CHECK(source);
		fread(source, 1, fsize, infile);
		fclose(infile);
		source[fsize] = 0;
		printf("\n%s",source);
		free(source);
		fclose(infile);
	}
	else
		printf("\nNo local help documentation found. Please refer to, https://github.com/TheRealMichaelWang/minima/wiki/Lists, for more information.");
	return 1;
}