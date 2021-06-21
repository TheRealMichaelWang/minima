#pragma once

#ifndef COLLECTION_H
#define COLLECTION_H

#include <stdint.h>

struct value; //foward declare value

struct collection {
	struct value** inner_collection;
	uint64_t size;
};

const int init_collection(struct collection* collection, uint64_t size);
void free_collection(struct collection* collection);

const int collection_compare(struct collection* a, struct collection* b);

#endif // !COLLECTION_H