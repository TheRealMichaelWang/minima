#pragma once

#ifndef LABEL_H
#define LABEL_H

#include <stdint.h>
#include "builtins/builtins.h"

struct global_cache {
	struct cache_bucket {
		uint64_t id;

		enum cache_type {
			CACHE_TYPE_POS,
			CACHE_TYPE_BUILTIN,
			CACHE_TYPE_PROTO,
			CACHE_TYPE_PROC
		}type;

		union payload
		{
			uint64_t pos;
			struct record_prototype* prototype; 
			DECL_BUILT_IN(*delegate);
		}payload;

		struct cache_bucket* next;
	}** buckets;
};

void init_global_cache(struct global_cache* global_cache);
void free_global_cache(struct global_cache* global_cache);

const int cache_insert_label(struct global_cache* global_cache, uint64_t id, uint64_t pos);
uint64_t cache_retrieve_pos(struct global_cache* global_cache, uint64_t id);

struct record_prototype* cache_insert_prototype(struct global_cache* global_cache, uint64_t id);
const int cache_init_record(struct global_cache* global_cache, uint64_t proto_id, struct record* record, struct machine* machine);
const int cache_merge_proto(struct global_cache* global_cache, uint64_t child, uint64_t parent);

const int cache_declare_builtin(struct global_cache* global_cache, uint64_t id, DECL_BUILT_IN(*delegate));
struct value cache_invoke_builtin(struct global_cache* global_cache, uint64_t id, struct value** argv, uint32_t argc, struct machine* machine);

#endif // !LABEL_H