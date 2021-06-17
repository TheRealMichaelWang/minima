#pragma once

#ifndef GARBAGE_H
#define GARBAGE_H

#include "value.h"

struct garbage_frame {
	struct value** to_collect;
	struct value** to_trace;
	unsigned long collect_values;
	unsigned long trace_values;
};

struct garbage_collector {
	struct garbage_frame* frame_stack;
	struct value** value_stack;
	struct value** trace_stack;
	unsigned long frames;
};

void init_gcollect(struct garbage_collector* garbage_collector);

void free_gcollect(struct garbage_collector* garbage_collector);

void gc_register_trace(struct garbage_collector* garbage_collector, struct value* value);

void gc_register_value(struct garbage_collector* garbage_collector, struct value* value, int noreg_head);

void gc_new_frame(struct garbage_collector* garbage_collector);

void gc_collect(struct garbage_collector* garbage_collector);

#endif // !GARBAGE_H
