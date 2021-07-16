#pragma once

#ifndef OBJECT_H
#define OBJECT_H

#include "collection.h"
#include "record.h"

struct object {
	union object_ptr
	{
		struct collection* collection;
		struct record* record;
	}ptr;

	enum obj_type
	{
		OBJ_TYPE_COL,
		OBJ_TYPE_REC
	} type;
};

void init_object_col(struct object* object, struct collection* collection);
void init_object_rec(struct object* object, struct record* record);
void free_object(struct object* object);

const int object_compare(const struct object* a, const struct object* b);
struct value**object_get_children(const struct object* object, uint64_t* size);

#endif // !OBJECT_H