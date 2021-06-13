#pragma once

#ifndef LABEL_H
#define LABEL_H

#include "record.h"

struct global_cache {
	struct cache_bucket {
		unsigned long id;

		enum cache_type {
			cache_type_position,
			cache_type_builtin,
			cache_type_prototype
		}type;

		union payload
		{
			unsigned long pos;
			struct record_prototype* prototype; 
			struct value* (*builtin_delegate)(struct value** argv, unsigned int argc);
		}payload;

		struct cache_bucket* next;
	}** buckets;
};

void init_global_cache(struct global_cache* global_cache);
void free_global_cache(struct global_cache* global_cache);

const int insert_label(struct global_cache* global_cache, unsigned long id, unsigned long pos);
unsigned long retrieve_pos(struct global_cache* global_cache, unsigned long id);

const int insert_prototype(struct global_cache* global_cache, unsigned long id, struct record_prototype* prototype);
const int init_record_id(struct global_cache* global_cache, unsigned long proto_id, struct record* record);

const int declare_builtin_proc(struct global_cache* global_cache, unsigned long id, struct value* (*delegate)(struct value** argv, unsigned int argc));
struct value* invoke_builtin(struct global_cache* global_cache, unsigned long id, struct value** argv, unsigned int argc);

#endif // !LABEL_H