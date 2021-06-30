#include <stdlib.h>
#include <string.h>
#include "include/runtime/object/object.h"

void init_object_col(struct object* object, struct collection* collection) {
	object->type = OBJ_TYPE_COL;
	object->ptr.collection = collection;
}

void init_object_rec(struct object* object, struct record* record) {
	object->type = OBJ_TYPE_REC;
	object->ptr.record = record;
}

void free_object(struct object* object) {
	switch (object->type)
	{
	case OBJ_TYPE_COL:
		free_collection(object->ptr.collection);
		free(object->ptr.collection);
		break;
	case OBJ_TYPE_REC:
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
	case OBJ_TYPE_COL:
		return !collection_compare(a->ptr.collection, b->ptr.collection);
	case OBJ_TYPE_REC:
		return !(a->ptr.record == b->ptr.record);
	}
	return 1;
}

const struct value** object_get_children(struct object* object, uint64_t* size) {
	switch (object->type)
	{
	case OBJ_TYPE_COL:
		*size = object->ptr.collection->size;
		return object->ptr.collection->inner_collection;
	case OBJ_TYPE_REC:
		*size = object->ptr.record->prototype->size;
		return object->ptr.record->properties;
	}
	return NULL;
}