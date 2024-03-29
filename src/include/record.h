#pragma once

#ifndef RECORD_H
#define RECORD_H

#include <stdint.h>

#define RECORD_BASE_PROPERTY 6385072256
#define RECORD_INIT_PROC 6385337657
#define RECORD_THIS 6385726429
#define RECORD_OVERLOAD_ORDER 7572763920046527

#define BINARY_OVERLOAD 2187
#define UNARY_OVERLOAD 3187

struct value;
struct machine;

struct record_prototype {
	uint64_t identifier;
	uint_fast8_t* property_map;
	uint_fast8_t size;
	struct record_prototype* base_prototype;
};

struct record {
	struct record_prototype* prototype;
	struct value** properties;
};

void init_record_prototype(struct record_prototype* prototype, uint64_t identifier);
void free_record_prototype(struct record_prototype* prototype);

const int record_append_property(struct record_prototype* prototype, uint64_t property);
const int record_inherit(struct record_prototype* child, struct record_prototype* parent);

const int init_record(struct record* record, struct record_prototype* prototype, struct machine* machine);
void free_record(struct record* record);

struct value* record_get_property(struct record* record, const uint64_t property);
const int record_set_property(struct record* record, const uint64_t property, const struct value* value);

#endif // !RECORD_H