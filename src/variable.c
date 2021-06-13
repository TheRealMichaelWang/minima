#include <stdlib.h>
#include "variable.h"

#define MAX_SIZE 63

int init_var_context(struct var_context* var_context, struct garbage_collector* garbage_collector) {
	var_context->buckets = calloc(MAX_SIZE, sizeof(struct var_bucket));
	var_context->garbage_collector = garbage_collector;
	gc_new_frame(garbage_collector);
	return 1;
}

void free_var_context(struct var_context* var_context) {
	for (unsigned char i = 0; i < MAX_SIZE; i++) {
		struct var_bucket* current = var_context->buckets[i];
		while (current != NULL)
		{
			struct var_bucket* old = current;
			current = current->next;
			free(old);
		}
	}
	gc_collect(var_context->garbage_collector);
	free(var_context->buckets);
}

const struct value* retrieve_var(struct var_context* var_context, const unsigned long id) {
	struct var_bucket* bucket = var_context->buckets[id & MAX_SIZE];
	while (bucket != NULL) {
		if (bucket->id_hash == id)
			return bucket->value;
		bucket = bucket->next;
	}
	return NULL;
}

int emplace_var(struct var_context* var_context, const unsigned long id, const struct value* value) {
	struct var_bucket** current_bucket = &var_context->buckets[id & MAX_SIZE];
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