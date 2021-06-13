#include <stdlib.h>
#include <string.h>
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
			if (bucket->type == cache_type_prototype) {
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

const int insert_bucket(struct global_cache* global_cache, struct cache_bucket to_insert) {
	struct cache_bucket** bucket = &global_cache->buckets[to_insert.id & MAX_SIZE];
	while (*bucket != NULL)
	{
		if ((*bucket)->id == to_insert.id && (*bucket)->type == to_insert.type)
			return 0;
		bucket = &(*bucket)->next;
	}
	*bucket = malloc(sizeof(struct cache_bucket));
	if (*bucket == NULL)
		return 0;
	memcpy(*bucket, &to_insert, sizeof(struct cache_bucket));
	(*bucket)->next = NULL;
	return 1;
}

const int insert_label(struct global_cache* global_cache, unsigned long id, unsigned long pos) {
	struct cache_bucket to_insert;
	to_insert.id = id;
	to_insert.type = cache_type_position;
	to_insert.payload.pos = pos;
	return insert_bucket(global_cache, to_insert);
}

unsigned long retrieve_pos(struct global_cache* global_cache, unsigned long id) {
	struct cache_bucket* bucket = global_cache->buckets[id & MAX_SIZE];
	while (bucket != NULL)
	{
		if (bucket->id == id && bucket->type == cache_type_position)
			return bucket->payload.pos;
		bucket = bucket->next;
	}
	return 0;
}

const int insert_prototype(struct global_cache* global_cache, unsigned long id, struct record_prototype* prototype) {
	struct cache_bucket to_insert;
	to_insert.id = id;
	to_insert.type = cache_type_prototype;
	to_insert.payload.prototype = prototype;
	return insert_bucket(global_cache, to_insert);
}

const int init_record_id(struct global_cache* global_cache, unsigned long proto_id, struct record* record) {
	struct cache_bucket* bucket = global_cache->buckets[proto_id & MAX_SIZE];
	while (bucket != NULL)
	{
		if (bucket->id == proto_id && bucket->type == cache_type_prototype) {
			init_record(record, bucket->payload.prototype);
			return 1;
		}
		bucket = bucket->next;
	}
	return 0;
}

const int declare_builtin_proc(struct global_cache* global_cache, unsigned long id, struct value* (*delegate)(struct value** argv, unsigned int argc)) {
	struct cache_bucket to_insert;
	to_insert.id = id;
	to_insert.type = cache_type_builtin;
	to_insert.payload.builtin_delegate = delegate;
	return insert_bucket(global_cache, to_insert);
}

struct value* invoke_builtin(struct global_cache* global_cache, unsigned long id, struct value** argv, unsigned int argc) {
	struct cache_bucket* bucket = global_cache->buckets[id & MAX_SIZE];
	while (bucket != NULL)
	{
		if (bucket->id == id && bucket->type == cache_type_builtin)
			return (*bucket->payload.builtin_delegate)(argv, argc);
		bucket = bucket->next;
	}
	return NULL;
}