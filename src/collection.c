#include <stdlib.h>
#include "error.h"
#include "value.h"
#include "collection.h"

const int init_collection(struct collection* collection, unsigned long size) {
	collection->inner_collection = malloc(size * sizeof(struct value*));
	ERROR_ALLOC_CHECK(collection->inner_collection);
	collection->size = size;
	return 1;
}

void free_collection(struct collection* collection) {
	for (unsigned long i = 0; i < collection->size; i++)
		if (collection->inner_collection[i]->gc_flag == GARBAGE_UNINIT) {
			free_value(collection->inner_collection[i]);
			free(collection->inner_collection[i]);
		}
	free(collection->inner_collection);
}

const int collection_compare(struct collection* a, struct collection* b) {
	if (a->size != b->size)
		return 0;
	for (unsigned long i = 0; i < a->size; i++) {
		if (compare_value(a, b))
			return 0;
	}
	return 1;
}