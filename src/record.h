#pragma once

#ifndef RECORD_H
#define RECORD_H

struct value; //forward declare value.

struct record_prototype {
	unsigned long identifier;
	unsigned char* property_map;
	unsigned char size;
};

struct record {
	struct record_prototype* prototype;
	struct value** properties;
};

void init_record_prototype(struct record_prototype* prototype, unsigned long identifier);
void free_record_prototype(struct record_prototype* prototype);

const int append_record_property(struct record_prototype* prototype, unsigned long property);

inline unsigned char retrieve_property_index(struct record_prototype* prototype, unsigned long property) {
	return prototype->property_map[property & 255];
}

void init_record(struct record* record, struct record_prototype* prototype);
void free_record(struct record* record);

struct value* get_value_ref(struct record* record, unsigned long property);

const int set_value_ref(struct record* record, unsigned long property, struct value* value);

#endif // !RECORD_H