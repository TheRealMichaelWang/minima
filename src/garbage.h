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
	init_gframe(&garbage_collector->frame_stack[garbage_collector->frames++], garbage_collector->frames == 0 ? garbage_collector->value_stack : &garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[garbage_collector->frame_stack[garbage_collector->frames - 1].values]);
}

#endif // !GARBAGE_H
