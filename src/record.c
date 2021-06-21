#include <stdlib.h>
#include "value.h"
#include "record.h"

#define MAX_SIZE 255

inline static const uint_fast8_t retrieve_property_index(struct record_prototype* prototype, uint64_t property) {
	return prototype->property_map[property & 255];
}

void init_record_prototype(struct record_prototype* prototype, uint64_t identifier) {
	prototype->identifier = identifier;
	prototype->property_map = calloc(MAX_SIZE, sizeof(uint_fast8_t));
	prototype->size = 0;
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

void init_record(struct record* record, struct record_prototype* prototype) {
	record->prototype = prototype;
	record->properties = malloc(prototype->size * sizeof(struct value*));
}

void free_record(struct record* record) {
	for (uint_fast8_t i = 0; i < record->prototype->size; i++)
		if (record->properties[i]->gc_flag == GARBAGE_UNINIT) {
			free_value(record->properties[i]);
			free(record->properties[i]);
		}
	free(record->properties);
}

struct value* record_get_ref(struct record* record, uint64_t property) {
	uint_fast8_t i = retrieve_property_index(record->prototype, property);
	if (i)
		return record->properties[i - 1];
	else
		return NULL;
}

const int record_set_ref(struct record* record, uint64_t property, struct value* value) {
	uint_fast8_t i = retrieve_property_index(record->prototype, property);
	if (i) {
		record->properties[i - 1] = value;
		return 1;
	}
	return 0;
}