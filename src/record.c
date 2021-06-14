#include <stdlib.h>
#include "value.h"
#include "record.h"

#define MAX_SIZE 255

void init_record_prototype(struct record_prototype* prototype, unsigned long identifier) {
	prototype->identifier = identifier;
	prototype->property_map = calloc(MAX_SIZE, sizeof(unsigned char));
	prototype->size = 0;
}

void free_record_prototype(struct record_prototype* prototype) {
	free(prototype->property_map);
}

const int append_record_property(struct record_prototype* prototype, unsigned long property) {
	unsigned char* slot = &prototype->property_map[property & 255];
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
	for (unsigned char i = 0; i < record->prototype->size; i++)
		if (record->properties[i]->gc_flag == garbage_uninit) {
			free_value(record->properties[i]);
			free(record->properties[i]);
		}
	free(record->properties);
}

struct value* get_value_ref(struct record* record, unsigned long property) {
	unsigned char i = retrieve_property_index(record->prototype, property);
	if (i)
		return record->properties[i - 1];
	else
		return NULL;
}

const int set_value_ref(struct record* record, unsigned long property, struct value* value) {
	unsigned char i = retrieve_property_index(record->prototype, property);
	if (i) {
		record->properties[i - 1] = value;
		return 1;
	}
	return 0;
}