#pragma once

#ifndef GARBAGE_H
#define GARBAGE_H

#include "value.h"

struct garbage_frame {
	struct value** to_collect;
	unsigned long values;
};

struct garbage_collector {
	struct garbage_frame* frame_stack;
	unsigned long frames;
};

void gc_collect(struct garbage_collector* garbage_collector);

void gc_protect(struct value* value);

void init_gframe(struct garbage_frame* garbage_frame);
void init_gcollect(struct garbage_collector* garbage_collector);

void free_gcollect(struct garbage_collector* garbage_collector);

void register_value(struct garbage_collector* garbage_collector, struct value* value, int noreg_head);

inline void new_gframe(struct garbage_collector* garbage_collector) {
	init_gframe(&garbage_collector->frame_stack[garbage_collector->frames++]);
}

#endif // !GARBAGE_H
