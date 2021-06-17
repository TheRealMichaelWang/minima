#pragma once

#ifndef LABEL_H
#define LABEL_H

#include "record.h"

struct global_cache {
	struct cache_bucket {
		unsigned long id;

		enum cache_type {
			CACHE_TYPE_POS,
			CACHE_TYPE_BUILTIN,
			CACHE_TYPE_PROTO
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

const int cache_insert_label(struct global_cache* global_cache, unsigned long id, unsigned long pos);
unsigned long cache_retrieve_pos(struct global_cache* global_cache, unsigned long id);

const int cache_insert_prototype(struct global_cache* global_cache, unsigned long id, struct record_prototype* prototype);
const int cache_init_record(struct global_cache* global_cache, unsigned long proto_id, struct record* record);

const int cache_declare_builtin(struct global_cache* global_cache, unsigned long id, struct value* (*delegate)(struct value** argv, unsigned int argc));
struct value* cache_invoke_builtin(struct global_cache* global_cache, unsigned long id, struct value** argv, unsigned int argc);

#endif // !LABEL_H