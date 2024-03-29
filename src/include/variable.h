#pragma once

#include "value.h"
#include "garbage.h"

#ifndef VARIABLE_H
#define VARIABLE_H

#include <stdint.h>

struct var_context {
	struct var_bucket
	{
		uint64_t identifier;
		struct value* value;
		int set_flag;
	}* buckets;

	uint64_t hash_limit;

	struct garbage_collector* garbage_collector;
};

const int init_var_context(struct var_context* var_context, struct garbage_collector* garbage_collector);
const int free_var_context(struct var_context* var_context);

struct value*retrieve_var(struct var_context* var_context, const uint64_t id);
const int emplace_var(struct var_context* var_context,const uint64_t id, struct value*value);

#endif // !VARIABLE_H
