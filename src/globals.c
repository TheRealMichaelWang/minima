#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "globals.h"

#define MAX_SIZE 255

void init_global_cache(struct global_cache* global_cache) {
	global_cache->buckets = calloc(MAX_SIZE, sizeof(struct cache_bucket*));
}

void free_global_cache(struct global_cache* global_cache) {
	for (unsigned char i = 0; i < MAX_SIZE; i++) {
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

const int cache_insert_label(struct global_cache* global_cache, unsigned long id, unsigned long pos) {
	struct cache_bucket to_insert;
	to_insert.id = id;
	to_insert.type = CACHE_TYPE_POS;
	to_insert.payload.pos = pos;
	return insert_bucket(global_cache, to_insert);
}

unsigned long cache_retrieve_pos(struct global_cache* global_cache, unsigned long id) {
	struct cache_bucket* bucket = global_cache->buckets[id & MAX_SIZE];
	while (bucket != NULL)
	{
		if (bucket->id == id && bucket->type == CACHE_TYPE_POS)
			return bucket->payload.pos;
		bucket = bucket->next;
	}
	return 0;
}

const int cache_insert_prototype(struct global_cache* global_cache, unsigned long id, struct record_prototype* prototype) {
	struct cache_bucket to_insert;
	to_insert.id = id;
	to_insert.type = CACHE_TYPE_PROTO;
	to_insert.payload.prototype = prototype;
	return insert_bucket(global_cache, to_insert);
}

const int cache_init_record(struct global_cache* global_cache, unsigned long proto_id, struct record* record) {
	struct cache_bucket* bucket = global_cache->buckets[proto_id & MAX_SIZE];
	while (bucket != NULL)
	{
		if (bucket->id == proto_id && bucket->type == CACHE_TYPE_PROTO) {
			init_record(record, bucket->payload.prototype);
			return 1;
		}
		bucket = bucket->next;
	}
	return 0;
}

const int cache_declare_builtin(struct global_cache* global_cache, unsigned long id, struct value* (*delegate)(struct value** argv, unsigned int argc)) {
	struct cache_bucket to_insert;
	to_insert.id = id;
	to_insert.type = CACHE_TYPE_BUILTIN;
	to_insert.payload.builtin_delegate = delegate;
	return insert_bucket(global_cache, to_insert);
}

struct value* cache_invoke_builtin(struct global_cache* global_cache, unsigned long id, struct value** argv, unsigned int argc) {
	struct cache_bucket* bucket = global_cache->buckets[id & MAX_SIZE];
	while (bucket != NULL)
	{
		if (bucket->id == id && bucket->type == CACHE_TYPE_BUILTIN)
			return (*bucket->payload.builtin_delegate)(argv, argc);
		bucket = bucket->next;
	}
	return NULL;
}