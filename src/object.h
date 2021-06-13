#pragma once

#ifndef OBJECT_H
#define OBJECT_H

struct value;

#include "collection.h"
#include "record.h"

struct object {
	union object_ptr
	{
		struct collection* collection;\
		struct record* record;
	}ptr;

	enum obj_type
	{
		obj_type_collection,
		obj_type_record
	} type;
};

void init_object_col(struct object* object, struct collection* collection);
void init_object_rec(struct object* object, struct record* record);
void free_object(struct object* object);

const int compare_object(struct object* a, struct object* b);
const struct value** get_children(struct object* object, unsigned long* size);

#endif // !OBJECT_H