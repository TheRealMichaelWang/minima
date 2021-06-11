#pragma once

#ifndef RECORD_H
#define RECORD_H

struct value; //forward declare value.

struct record_prototye {
	unsigned long* property_map;
	unsigned char size;
};

struct record {
	struct record_prototype* prototype;
	struct value** properties;
};

void init_record_prototype(struct record_prototype* prototype);
void free_record_prototype(struct record_prototype* prototype);

void append_record_property(struct record_prototype* prototype, unsigned long property);

void init_record(struct record* record, struct record_prototype* prototype);
void free_record(struct record* record);

struct value* get_value_ref(struct record* record, unsigned long property);
struct value* set_value_ref(struct record* record, unsigned long property)

#endif // !RECORD_H