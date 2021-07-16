#include <stdlib.h>
#include "include/error.h"
#include "include/runtime/value.h"
#include "include/runtime/object/collection.h"

const int init_collection(struct collection* collection, uint64_t size) {
	collection->inner_collection = malloc(size * sizeof(struct value*));
	ERROR_ALLOC_CHECK(collection->inner_collection);
	collection->size = size;
	return 1;
}

void free_collection(struct collection* collection) {
	free(collection->inner_collection);
}

const int collection_compare(const struct collection* a, const struct collection* b) {
	if (a->size != b->size)
		return 0;
	for (uint64_t i = 0; i < a->size; i++) {
		if (compare_value(a->inner_collection[i], b->inner_collection[i]))
			return 0;
	}
	return 1;
}