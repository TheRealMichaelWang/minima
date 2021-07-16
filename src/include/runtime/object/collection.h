#pragma once

#ifndef COLLECTION_H
#define COLLECTION_H

#include <stdint.h>

struct value; //foward declare value

struct collection {
	struct value** inner_collection;
	uint64_t size;
};

//initializes a new collection instance. Note that the collection's elements remain uninitialized
const int init_collection(struct collection* collection, uint64_t size);

//frees up a collection instances resources
void free_collection(struct collection* collection);

//compares a collection - returns 0 if they are the same, returns the difference otherwise
const int collection_compare(const struct collection* a, const struct collection* b);

#endif // !COLLECTION_H