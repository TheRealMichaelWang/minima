#pragma once

#ifndef COLLECTION_H
#define COLLECTION_H

struct value; //foward declare value

struct collection {
	struct value** inner_collection;
	unsigned long size;
};

const int init_collection(struct collection* collection, unsigned long size);
void free_collection(struct collection* collection);

const int copy_collection(struct collection* dest, struct collection* src);
const int compare_collection(struct collection* a, struct collection* b);

#endif // !COLLECTION_H