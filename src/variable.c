#include <stdlib.h>
#include <string.h>
#include "include/error.h"
#include "include/variable.h"

const int init_var_context(struct var_context* var_context, struct garbage_collector* garbage_collector) {
	var_context->hash_limit = 8;
	var_context->garbage_collector = garbage_collector;
	ERROR_ALLOC_CHECK(var_context->buckets = malloc(var_context->hash_limit * sizeof(struct var_bucket)));
	
	for (uint_fast64_t i = 0; i < var_context->hash_limit; i++)
		var_context->buckets[i].set_flag = 0;
	gc_new_frame(garbage_collector);
	
	return 1;
}

const int free_var_context(struct var_context* var_context) {
	free(var_context->buckets);
	return gc_collect(var_context->garbage_collector);
}

static const int rehash(struct var_context* var_context) {
	struct var_bucket* old_buckets = var_context->buckets;
	uint64_t old_limit = var_context->hash_limit;

	var_context->hash_limit *= 2;

	ERROR_ALLOC_CHECK(var_context->buckets = malloc(var_context->hash_limit * sizeof(struct var_bucket)));
	for (uint_fast64_t i = 0; i < var_context->hash_limit; i++)
		var_context->buckets[i].set_flag = 0;

	for (uint_fast64_t i = 0; i < old_limit; i++)
		if (old_buckets[i].set_flag)
			emplace_var(var_context, old_buckets[i].identifier, old_buckets[i].value);

	free(old_buckets);
	return 1;
}

struct value*retrieve_var(struct var_context* var_context, const uint64_t id) {
	for (uint_fast64_t i = id & (var_context->hash_limit - 1); i < var_context->hash_limit && var_context->buckets[i].set_flag; i++) {
		if (var_context->buckets[i].identifier == id)
			return var_context->buckets[i].value;
	}
	return NULL;
}

const int emplace_var(struct var_context* var_context, const uint64_t id, struct value*value) {
	ERROR_ALLOC_CHECK(value);

	for (uint_fast64_t i = id & (var_context->hash_limit - 1); i < var_context->hash_limit; i++) {
		if (!var_context->buckets[i].set_flag || (var_context->buckets[i].identifier == id)) {
			var_context->buckets[i].identifier = id;
			var_context->buckets[i].value = value;
			var_context->buckets[i].set_flag = 1;
			return 1;
		}
	}
	
	if (!rehash(var_context))
		return 0;
	return emplace_var(var_context, id, value);
}