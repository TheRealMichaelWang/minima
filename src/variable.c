#include <stdlib.h>
#include "variable.h"

#define VAR_MAX_BUCKETS 250

int init_var_context(struct var_context* var_context, struct garbage_collector* garbage_collector) {
	var_context->buckets = calloc(VAR_MAX_BUCKETS, sizeof(struct var_bucket*));
	if (var_context->buckets == NULL)
		return 0;
	var_context->garbage_collector = garbage_collector;
	new_gframe(garbage_collector);
	return 1;
}

void free_var_context(struct var_context* var_context) {
	for (unsigned char i = 0; i < VAR_MAX_BUCKETS; i++) {
		struct var_bucket* current = var_context->buckets[i];
		while (current != NULL)
		{
			struct var_bucket* old = current;
			current = current->next;
			free(old);
		}
	}
	free(var_context->buckets);
	gc_collect(var_context->garbage_collector);
}

const struct value* retrieve_var(struct var_context* var_context, const unsigned long id) {
	struct var_bucket* bucket = var_context->buckets[id % VAR_MAX_BUCKETS];
	while (bucket != NULL) {
		if (bucket->id_hash == id)
			return bucket->value;
		bucket = bucket->next;
	}
	return NULL;
}

int emplace_var(struct var_context* var_context, const unsigned long id, const struct value* value) {
	struct var_bucket** current_bucket = &var_context->buckets[id % VAR_MAX_BUCKETS];
	while (*current_bucket != NULL)
	{
		if ((*current_bucket)->id_hash == id)
			break;
		current_bucket = &(*current_bucket)->next;
	}
	if (*current_bucket == NULL) {
		*current_bucket = malloc(sizeof(struct var_bucket));
		if (*current_bucket == NULL)
			return 0;
		(*current_bucket)->id_hash = id;
		(*current_bucket)->next = NULL;
	}
	(*current_bucket)->value = value;
	return 1;
}