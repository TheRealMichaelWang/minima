#pragma once

#include "value.h"
#include "garbage.h"

#ifndef VARIABLE_H
#define VARIABLE_H

#include <stdint.h>

struct var_context {
	struct var_bucket {
		uint64_t id_hash;
		const struct value* value;
		struct var_bucket* next;
	} **buckets;
	struct garbage_collector* garbage_collector;
};

int init_var_context(struct var_context* var_context, struct garbage_collector* garbage_collector);
void free_var_context(struct var_context* var_context);

const struct value* retrieve_var(struct var_context* var_context, const uint64_t id);

const int emplace_var(struct var_context* var_context,const uint64_t id, const struct value* value);


#endif // !VARIABLE_H
