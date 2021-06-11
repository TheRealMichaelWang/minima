#include <stdlib.h>
#include "value.h"
#include "collection.h"

const int init_collection(struct collection* collection, unsigned long size) {
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