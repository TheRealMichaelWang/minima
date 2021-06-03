#pragma once

#ifndef OBJECT_H
#define OBJECT_H

#include "value.h"
#include "garbage.h"

struct collection {
	struct value** inner_collection;
	unsigned long size;
};

int init_collection(struct collection* collection, unsigned long size, struct garbage_collector* garbage_collector);
void free_collection(struct collection* collection);

int copy_collection(struct collection* dest, struct collection* src);
const int compare_collection(struct collection* a, struct collection* b);

#endif // !OBJECT_H
