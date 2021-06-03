#include <stdlib.h>
#include "collection.h"

int init_collection(struct collection* collection, unsigned long size, struct garbage_collector* garbage_collector) {
	collection->inner_collection = malloc(size * sizeof(struct value*));
	if (collection->inner_collection == NULL)
		return 0;
	collection->size = size;
	for (unsigned long i = 0; i < size; i++) {
		struct value* my_val = malloc(sizeof(struct value));
		if (my_val == NULL)
			return 0;
		init_value(my_val, VALUE_TYPE_NULL, NULL);
		collection->inner_collection[i] = my_val;
		register_value(garbage_collector, my_val);
	}
	return 1;
}

void free_collection(struct collection* collection) {
	free(collection->inner_collection);
}

int copy_collection(struct collection* dest, struct collection* src) {
	dest->size = src->size;
	dest->inner_collection = malloc(dest->size * sizeof(struct value*));
	if (dest->inner_collection == NULL)
		return 0;
	for (unsigned long i = 0; i < dest->size; i++)
		if (!copy_value(dest->inner_collection[i], src->inner_collection[i]))
			return 0;
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