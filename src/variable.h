#pragma once

#include "value.h"
#include "garbage.h"

#ifndef VARIABLE_H
#define VARIABLE_H

struct var_context {
	struct var_bucket {
		unsigned long id_hash;
		const struct value* value;
		struct var_bucket* next;
	}* buckets[63];
	struct garbage_collector* garbage_collector;
};

int init_var_context(struct var_context* var_context, struct garbage_collector* garbage_collector);
void free_var_context(struct var_context* var_context);

const struct value* retrieve_var(struct var_context* var_context, const unsigned long id);

int emplace_var(struct var_context* var_context,const unsigned long id, const struct value* value);


#endif // !VARIABLE_H
