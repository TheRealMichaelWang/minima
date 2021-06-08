#include <stdlib.h>
#include <string.h>
#include "value.h"

void init_null(struct value* value) {
	value->gc_flag = garbage_uninit;
	value->type = null;
}

void init_num(struct value* value, const double num) {
	value->gc_flag = garbage_uninit;
	value->type = numerical;
	value->payload.numerical = num;
}

void init_char(struct value* value, const char c) {
	value->gc_flag = garbage_uninit;
	value->type = character;
	value->payload.character = c;
}

void init_col(struct value* value, struct collection* col) {
	value->gc_flag = garbage_uninit;
	value->type = collection;
	value->payload.collection = col;
}

const int copy_value(struct value* dest, struct value* src) {
	dest->type = src->type;
	memcpy(dest, src, sizeof(struct value));
	dest->gc_flag = garbage_uninit;
	if (dest->type == collection) {
		if (!copy_collection(dest->payload.collection, src->payload.collection))
			return 0;
	}
	return 1;
}

const int compare_value(struct value* a, struct value* b) {
	if (a->type != b->type)
		return a->type - b->type;
	switch (a->type)
	{
	case null:
		return 1;
	case numerical: {
		return a->payload.numerical - b->payload.numerical;
	}
	case character:
		return a->payload.character - b->payload.character;
	case collection: {
		return !compare_collection(a->payload.collection, b->payload.collection);
	}
	}
	return 0;
}

void free_value(struct value* value) {
	if (value->type == collection) {
		free_collection(value->payload.collection);
		free(value->payload.collection);
	}
}

int init_collection(struct collection* collection, unsigned long size) {
	collection->inner_collection = malloc(size * sizeof(struct value*));
	if (collection->inner_collection == NULL)
		return 0;
	collection->size = size;
	return 1;
}

void free_collection(struct collection* collection) {
	for (unsigned long i = 0; i < collection->size; i++)
		if (collection->inner_collection[i]->gc_flag == garbage_uninit) {
			free_value(collection->inner_collection[i]);
			free(collection->inner_collection[i]);
		}
	free(collection->inner_collection);
}

const int copy_collection(struct collection* dest, struct collection* src) {
	dest->size = src->size;
	dest->inner_collection = malloc(dest->size * sizeof(struct value*));
	if (dest->inner_collection == NULL)
		return 0;
	for (unsigned long i = 0; i < dest->size; i++) {
		dest->inner_collection[i] = malloc(sizeof(struct value));
		if (dest->inner_collection[i] == NULL)
			return 0;
		copy_value(dest->inner_collection[i], src->inner_collection[i]);
	}
	return 1;
}

const int compare_collection(struct collection* a, struct collection* b) {
	if (a->size != b->size)
		return 0;
	for (unsigned long i = 0; i < a->size; i++) {
		if (compare_value(a, b))
			return 0;
	}
	return 1;
}