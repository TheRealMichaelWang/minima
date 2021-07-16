#pragma once

#ifndef GARBAGE_H
#define GARBAGE_H

#include <stdint.h>
#include "value.h"

struct garbage_frame {
	struct value** to_collect;
	struct value** to_trace;
	uint64_t collect_values;
	uint64_t trace_values;
};

struct garbage_collector {
	struct garbage_frame* frame_stack;
	struct value** value_stack;
	struct value** trace_stack;
	uint64_t frames;
};

const int init_gcollect(struct garbage_collector* garbage_collector);
void free_gcollect(struct garbage_collector* garbage_collector);

const int gc_register_trace(struct garbage_collector* garbage_collector, const struct value* value);
const int gc_register_children(struct garbage_collector* garbage_collector, struct value* head);
struct value*gc_register_value(struct garbage_collector* garbage_collector, struct value value);

void gc_new_frame(struct garbage_collector* garbage_collector);
void gc_collect(struct garbage_collector* garbage_collector);

#endif // !GARBAGE_H
