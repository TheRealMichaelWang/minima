#pragma once

#ifndef OBJECT_H
#define OBJECT_H

struct value;

#include "collection.h"

struct object {
	union object_ptr
	{
		struct collection* collection;
	}ptr;

	enum obj_type
	{
		obj_type_collection
	} type;
};

void init_object_col(struct object* object, struct collection* collection);
void free_object(struct object* object);

const int copy_object(struct object* dest, struct object* src);
const int compare_object(struct object* a, struct object* b);
const struct value** get_children(struct object* object, unsigned long* size);

#endif // !OBJECT_H