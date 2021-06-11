#include <stdlib.h>
#include "value.h"
#include "record.h"

#define MAX_SIZE 255

void init_record_prototype(struct record_prototype* prototype) {
	prototype->property_map = malloc(MAX_SIZE * sizeof(unsigned char));
	prototype->size = 0;
}

void free_record_prototype(struct record_prototype* prototype) {
	free(prototype->property_map);
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