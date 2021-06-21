#pragma once

#ifndef RECORD_H
#define RECORD_H

#include <stdint.h>

struct value; //forward declare value.

struct record_prototype {
	uint64_t identifier;
	unsigned char* property_map;
	unsigned char size;
};

struct record {
	struct record_prototype* prototype;
	struct value** properties;
};

void init_record_prototype(struct record_prototype* prototype, uint64_t identifier);
void free_record_prototype(struct record_prototype* prototype);

const int record_append_property(struct record_prototype* prototype, uint64_t property);

void init_record(struct record* record, struct record_prototype* prototype);
void free_record(struct record* record);

struct value* record_get_ref(struct record* record, uint64_t property);

const int record_set_ref(struct record* record, uint64_t property, struct value* value);

#endif // !RECORD_H