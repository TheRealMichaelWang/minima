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
	struct value** value_stack;
	unsigned long frames;
};

void gc_collect(struct garbage_collector* garbage_collector);

void gc_protect(struct value* value);

void init_gcollect(struct garbage_collector* garbage_collector);

void free_gcollect(struct garbage_collector* garbage_collector);

void init_gframe(struct garbage_frame* garbage_frame, struct value** begin);

void gc_register_value(struct garbage_collector* garbage_collector, struct value* value, int noreg_head);

inline void gc_new_frame(struct garbage_collector* garbage_collector) {
	struct value** begin = garbage_collector->value_stack;
	if (garbage_collector->frames > 0) {
		struct garbage_frame* prev_frame = &garbage_collector->frame_stack[garbage_collector->frames - 1];
		begin = &prev_frame->to_collect[prev_frame->values];
	}
	init_gframe(&garbage_collector->frame_stack[garbage_collector->frames++], begin);
}

#endif // !GARBAGE_H
