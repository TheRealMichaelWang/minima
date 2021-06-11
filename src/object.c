#include <stdlib.h>
#include <string.h>
#include "object.h"

void init_object_col(struct object* object, struct collection* collection) {
	object->type = obj_type_collection;
	object->ptr.collection = collection;
}

void free_object(struct object* object) {
	if (object->type == obj_type_collection) {
		free_collection(object->ptr.collection);
		free(object->ptr.collection);
	}
}

const int copy_object(struct object* dest, struct object* src) {
	memcpy(dest, src, sizeof(struct object));
	switch (dest->type)
	{
	case obj_type_collection:
		if (!copy_collection(dest->ptr.collection, src->ptr.collection))
			return 0;
		break;
	}
	return 1;
}

const int compare_object(struct object* a, struct object* b) {
	if (a->type != b->type)
		return a->type = b->type;
	switch (a->type)
	{
	case obj_type_collection:
		return !compare_collection(a->ptr.collection, b->ptr.collection);
	}
	return 1;
}

const struct value** get_children(struct object* object, unsigned long* size) {
	switch (object->type)
	{
	case obj_type_collection:
		*size = object->ptr.collection->size;
		return object->ptr.collection->inner_collection;
	}
	return NULL;
}