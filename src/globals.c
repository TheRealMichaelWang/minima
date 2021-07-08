#include <stdlib.h>
#include <string.h>
#include "include/error.h"
#include "include/runtime/globals.h"

#define MAX_SIZE 255

void init_global_cache(struct global_cache* global_cache) {
	global_cache->buckets = calloc(MAX_SIZE, sizeof(struct cache_bucket*));
}

void free_global_cache(struct global_cache* global_cache) {
	for (uint_fast8_t i = 0; i < MAX_SIZE; i++) {
		struct cache_bucket* bucket = global_cache->buckets[i];
		while (bucket != NULL)
		{
			if (bucket->type == CACHE_TYPE_PROTO) {
				free_record_prototype(bucket->payload.prototype);
				free(bucket->payload.prototype);
			}
			struct cache_bucket* old = bucket;
			bucket = bucket->next;
			free(old);
		}
	}
	free(global_cache->buckets);
}

static const int insert_bucket(struct global_cache* global_cache, struct cache_bucket to_insert) {
	struct cache_bucket** bucket = &global_cache->buckets[to_insert.id & MAX_SIZE];
	while (*bucket != NULL)
	{
		if ((*bucket)->id == to_insert.id && (*bucket)->type == to_insert.type)
			return 0;
		bucket = &(*bucket)->next;
	}
	*bucket = malloc(sizeof(struct cache_bucket));
	ERROR_ALLOC_CHECK(*bucket);
	memcpy(*bucket, &to_insert, sizeof(struct cache_bucket));
	(*bucket)->next = NULL;
	return 1;
}

static struct cache_bucket* get_cache_bucket(struct global_cache* global_cache, uint64_t id, enum cache_type type) {
	struct cache_bucket* bucket = global_cache->buckets[id & MAX_SIZE];
	while (bucket != NULL)
	{
		if (bucket->id == id && bucket->type == type)
			return bucket;
		bucket = bucket->next;
	}
	return NULL;
}

const int cache_insert_label(struct global_cache* global_cache, uint64_t id, uint64_t pos) {
	struct cache_bucket to_insert;
	to_insert.id = id;
	to_insert.type = CACHE_TYPE_POS;
	to_insert.payload.pos = pos;
	return insert_bucket(global_cache, to_insert);
}

uint64_t cache_retrieve_pos(struct global_cache* global_cache, uint64_t id) {
	struct cache_bucket* bucket = get_cache_bucket(global_cache, id, CACHE_TYPE_POS);
	if (bucket)
		return bucket->payload.pos;
	return 0;
}

const int cache_insert_prototype(struct global_cache* global_cache, uint64_t id, struct record_prototype* prototype) {
	struct cache_bucket to_insert;
	to_insert.id = id;
	to_insert.type = CACHE_TYPE_PROTO;
	to_insert.payload.prototype = prototype;
	return insert_bucket(global_cache, to_insert);
}

const int cache_init_record(struct global_cache* global_cache, uint64_t proto_id, struct record* record, struct machine* machine) {
	struct cache_bucket* bucket = get_cache_bucket(global_cache, proto_id, CACHE_TYPE_PROTO);
	if (bucket) {
		init_record(record, bucket->payload.prototype, machine);
		return 1;
	}
	return 0;
}

const int cache_merge_proto(struct global_cache* global_cache, uint64_t child, uint64_t parent) {
	struct cache_bucket* child_bucket = get_cache_bucket(global_cache, child, CACHE_TYPE_PROTO);
	struct cache_bucket* parent_bucket = get_cache_bucket(global_cache, parent, CACHE_TYPE_PROTO);
	if (child_bucket && parent_bucket) {
		record_inherit(child_bucket->payload.prototype, parent_bucket->payload.prototype);
		return 1;
	}
	return 0;
}

const int cache_declare_builtin(struct global_cache* global_cache, uint64_t id, struct value* (*delegate)(struct value** argv, uint32_t argc)) {
	struct cache_bucket to_insert;
	to_insert.id = id;
	to_insert.type = CACHE_TYPE_BUILTIN;
	to_insert.payload.builtin_delegate = delegate;
	return insert_bucket(global_cache, to_insert);
}

struct value* cache_invoke_builtin(struct global_cache* global_cache, uint64_t id, struct value** argv, uint32_t argc) {
	struct cache_bucket* bucket = get_cache_bucket(global_cache, id, CACHE_TYPE_BUILTIN);
	if (bucket)
		return (*bucket->payload.builtin_delegate)(argv, argc);
	return NULL;
}