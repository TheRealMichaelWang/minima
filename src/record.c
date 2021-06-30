#include <stdlib.h>
#include "include/error.h"
#include "include/runtime/value.h"

#define MAX_SIZE 255

inline static const uint8_t retrieve_property_index(struct record_prototype* prototype, uint64_t property) {
	return prototype->property_map[property & 255];
}

void init_record_prototype(struct record_prototype* prototype, uint64_t identifier) {
	prototype->identifier = identifier;
	prototype->property_map = calloc(MAX_SIZE, sizeof(uint_fast8_t));
	prototype->size = 0;
	prototype->base_prototype = NULL;
}

void free_record_prototype(struct record_prototype* prototype) {
	free(prototype->property_map);
}

const int record_append_property(struct record_prototype* prototype, uint64_t property) {
	uint_fast8_t* slot = &prototype->property_map[property & 255];
	if (*slot)
		return 0;
	*slot = 1 + prototype->size++;
	return 1;
}

const int record_inherit(struct record_prototype* child, struct record_prototype* parent) {
	if (child->base_prototype)
		return 0;
	child->base_prototype = parent;
	record_append_property(child, RECORD_BASE_PROPERTY);
	return 1;  
}

const int init_record(struct record* record, struct record_prototype* prototype) {
	record->prototype = prototype;
	ERROR_ALLOC_CHECK(record->properties = malloc(prototype->size * sizeof(struct value*)));
	for (uint_fast8_t i = 0; i < prototype->size; i++) {
		struct value* property = malloc(sizeof(struct value));
		ERROR_ALLOC_CHECK(property);
		if (prototype->base_prototype && i == retrieve_property_index(prototype, RECORD_BASE_PROPERTY) - 1) {
			struct record* record = malloc(sizeof(struct record));
			ERROR_ALLOC_CHECK(record);
			init_record(record, prototype->base_prototype);
			struct object object;
			init_object_rec(&object, record);
			init_obj_value(property, object);
		}
		else 
			init_null_value(property);
		record->properties[i] = property;
	}
	return 1;
}

void free_record(struct record* record) {
	for (uint_fast8_t i = 0; i < record->prototype->size; i++)
		if (record->properties[i]->gc_flag == GARBAGE_UNINIT) {
			free_value(record->properties[i]);
			free(record->properties[i]);
		}
	free(record->properties);
}

struct value* record_get_property(struct record* record, const uint64_t property) {
	uint_fast8_t i = retrieve_property_index(record->prototype, property);
	if (i)
		return record->properties[i - 1];
	else if (record->prototype->base_prototype) {
		struct value* record_val = record_get_property(record, RECORD_BASE_PROPERTY);
		if (record_val && IS_RECORD(record_val)) {
			return record_get_property(record_val->payload.object.ptr.record, property);
		}
	}
	return NULL;
}

const int record_set_property(struct record* record, const uint64_t property, struct value* value) {
	uint_fast8_t i = retrieve_property_index(record->prototype, property);
	if (i) {
		record->properties[i - 1] = value;
		return 1;
	}
	else if (record->prototype->base_prototype) {
		struct value* record_val = record_get_property(record, RECORD_BASE_PROPERTY);
		if (record_val && IS_RECORD(record_val)) {
			return record_set_property(record_val, property, value);
		}
	}
	return 0;
}