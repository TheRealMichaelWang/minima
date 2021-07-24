#include <stdlib.h>
#include "include/io.h"
#include "include/error.h"
#include "include/runtime/object/object.h"
#include "include/runtime/garbage.h"

#define MAX_FRAMES 10000
#define MAX_VALUES 300000
#define MAX_TRACE 25000

const int init_gcollect(struct garbage_collector* garbage_collector) {
	ERROR_ALLOC_CHECK(garbage_collector->frame_stack = malloc(MAX_FRAMES * sizeof(struct garbage_frame)));
	ERROR_ALLOC_CHECK(garbage_collector->value_stack = malloc(MAX_VALUES * sizeof(struct value*)));
	ERROR_ALLOC_CHECK(garbage_collector->trace_stack = malloc(MAX_TRACE * sizeof(struct value*)));
	garbage_collector->frames = 0;
	garbage_collector->to_collect = 0;
	garbage_collector->to_trace = 0;
	return 1;
}

void free_gcollect(struct garbage_collector* garbage_collector) {
	while (garbage_collector->frames > 0) {
		if (!gc_collect(garbage_collector))
			garbage_collector->frames--;
	}
	free(garbage_collector->frame_stack);
	free(garbage_collector->value_stack);
	free(garbage_collector->trace_stack);
}

static void init_gframe(struct garbage_frame* garbage_frame, const struct value** value_begin, const struct value** trace_begin) {
	garbage_frame->to_collect = value_begin;
	garbage_frame->to_trace = trace_begin;
	garbage_frame->collect_values = 0;
	garbage_frame->trace_values = 0;
}

const int gc_register_trace(struct garbage_collector* garbage_collector, const struct value* value) {
	if (garbage_collector->to_trace == MAX_TRACE)
		return 0;
	struct garbage_frame* top = &garbage_collector->frame_stack[garbage_collector->frames - 1];
	if (top->trace_values == MAX_FRAMES)
		return 0;
	top->to_trace[top->trace_values++] = value;
	garbage_collector->to_trace++;
	return 1;
}

const int gc_register_children(struct garbage_collector* garbage_collector, struct value* head) {
	if (!head || head->gc_flag == GARBAGE_COLLECT)
		return 0;
	head->gc_flag = GARBAGE_COLLECT;
	
	if (head->type == VALUE_TYPE_OBJ) {
		uint64_t size = 0;
		struct value** children = object_get_children(&head->payload.object, &size);

		for (uint64_t i = 0; i < size; i++)
			if (children[i]->gc_flag == GARBAGE_UNINIT) {
				children[i] = gc_register_value(garbage_collector, *children[i]);
				if (!children[i]) {
					children[i] = NULL;
					return NULL;
				}
			}
	}
	return 1;
}

struct value*gc_register_value(struct garbage_collector* garbage_collector, struct value value) {
	if (value.gc_flag != GARBAGE_UNINIT || garbage_collector->to_collect == MAX_VALUES)
		return NULL;

	struct value* alloc_apartment = malloc(sizeof(struct value));

	ERROR_ALLOC_CHECK(alloc_apartment);
	*alloc_apartment = value;

	struct garbage_frame* gframe = &garbage_collector->frame_stack[garbage_collector->frames - 1];
	gframe->to_collect[gframe->collect_values++] = alloc_apartment;
	garbage_collector->to_collect++;
	
	ERROR_ALLOC_CHECK(gc_register_children(garbage_collector, alloc_apartment));
	return alloc_apartment;
}

void gc_new_frame(struct garbage_collector* garbage_collector) {
	struct value** begin = garbage_collector->value_stack;
	struct value** trace_begin = garbage_collector->trace_stack;
	if (garbage_collector->frames > 0) {
		struct garbage_frame* prev_frame = &garbage_collector->frame_stack[garbage_collector->frames - 1];
		begin = &prev_frame->to_collect[prev_frame->collect_values];
		trace_begin = &prev_frame->to_trace[prev_frame->trace_values];
	}
	init_gframe(&garbage_collector->frame_stack[garbage_collector->frames++], begin, trace_begin);
}

static const int trace_value(struct garbage_collector* garbage_collector, struct value* value, struct value** reset_stack, uint32_t* stack_top) {
	ERROR_ALLOC_CHECK(value);
	if (value->gc_flag != GARBAGE_UNINIT) {
		if (value->gc_flag == GARBAGE_KEEP)
			return 1;

		value->gc_flag = GARBAGE_KEEP;

		if (garbage_collector->to_trace == MAX_TRACE)
			return 0;

		reset_stack[*stack_top] = value;
		(*stack_top)++;
		garbage_collector->to_trace++;
	}
	if (value->type == VALUE_TYPE_OBJ) {
		uint64_t size = 0;
		struct value**children = object_get_children(&value->payload.object, &size);
		for (uint64_t i = 0; i < size; i++)
			ERROR_ALLOC_CHECK(trace_value(garbage_collector, children[i], reset_stack, stack_top));
	}
	return 1;
}

const int gc_collect(struct garbage_collector* garbage_collector) {
	struct garbage_frame* top = &garbage_collector->frame_stack[garbage_collector->frames - 1];

	struct value** reset_stack = &top->to_trace[top->trace_values];
	uint32_t reset_top = 0;

	int err_flag = 0;

	for (uint_fast64_t i = 0; i < top->trace_values; i++)
		if (!trace_value(garbage_collector, top->to_trace[i], reset_stack, &reset_top)) {
			err_flag = 1;
			break;
		}

	for (uint64_t i = 0; i < top->collect_values; i++) {
		if (garbage_collector->frames < 2 || (top->to_collect[i]->gc_flag == GARBAGE_COLLECT && !err_flag)) {
			free_value(top->to_collect[i]);
			free(top->to_collect[i]);
			garbage_collector->to_collect--;
		}
		else {
			struct garbage_frame* prev = &garbage_collector->frame_stack[garbage_collector->frames - 2];
			prev->to_collect[prev->collect_values++] = top->to_collect[i];
			garbage_collector->to_collect++;
		}
	}

	garbage_collector->to_trace -= reset_top;
	while (reset_top--)
		reset_stack[reset_top]->gc_flag = GARBAGE_COLLECT;

	garbage_collector->to_trace -= top->trace_values;
	garbage_collector->frames--;
	return 1;
}