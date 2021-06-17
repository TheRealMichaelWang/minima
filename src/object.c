#include <stdlib.h>
#include <string.h>
#include "object.h"

void init_object_col(struct object* object, struct collection* collection) {
	object->type = obj_type_collection;
	object->ptr.collection = collection;
}

void init_object_rec(struct object* object, struct record* record) {
	object->type = obj_type_record;
	object->ptr.record = record;
}

void free_object(struct object* object) {
	switch (object->type)
	{
	case obj_type_collection:
		free_collection(object->ptr.collection);
		free(object->ptr.collection);
		break;
	case obj_type_record:
		free_record(object->ptr.record);
		free(object->ptr.record);
		break;
	}
}

const int object_compare(struct object* a, struct object* b) {
	if (a->type != b->type)
		return a->type = b->type;
	switch (a->type)
	{
	case obj_type_collection:
		return !collection_compare(a->ptr.collection, b->ptr.collection);
	case obj_type_record:
		return !(a->ptr.record == b->ptr.record);
	}
	return 1;
}

const struct value** object_get_children(struct object* object, unsigned long* size) {
	switch (object->type)
	{
	case obj_type_collection:
		*size = object->ptr.collection->size;
		return object->ptr.collection->inner_collection;
	case obj_type_record:
		*size = object->ptr.record->prototype->size;
		return object->ptr.record->properties;
	}
	return NULL;
}