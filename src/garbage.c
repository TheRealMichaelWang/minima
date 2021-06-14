#include <stdlib.h>
#include "object.h"
#include "garbage.h"

#define MAX_GARBAGE 1000
#define MAX_VALUES 20000
#define MAX_TRACE 5000

void init_gcollect(struct garbage_collector* garbage_collector) {
	garbage_collector->frame_stack = malloc(MAX_GARBAGE * sizeof(struct garbage_frame));
	garbage_collector->value_stack = malloc(MAX_VALUES * sizeof(struct value*));
	garbage_collector->trace_stack = malloc(MAX_TRACE * sizeof(struct value*));
	garbage_collector->frames = 0;
}

void free_gcollect(struct garbage_collector* garbage_collector) {
	while (garbage_collector->frames > 0)
		gc_collect(garbage_collector);
	free(garbage_collector->frame_stack);
	free(garbage_collector->value_stack);
	free(garbage_collector->trace_stack);
}

void init_gframe(struct garbage_frame* garbage_frame, struct value** value_begin, struct value** trace_begin) {
	garbage_frame->to_collect = value_begin;
	garbage_frame->to_trace = trace_begin;
	garbage_frame->collect_values = 0;
	garbage_frame->trace_values = 0;
}

void gc_register_trace(struct garbage_collector* garbage_collector, struct value* value) {
	value->gc_flag = garbage_trace;
	struct garbage_frame* top = &garbage_collector->frame_stack[garbage_collector->frames - 1];
	if (top->to_trace == MAX_GARBAGE)
		return 0;
	top->to_trace[top->trace_values++] = value;
}

void gc_register_value(struct garbage_collector* garbage_collector, struct value* value, const int noreg_head) {
	if (value->gc_flag != garbage_uninit)
		return;
	value->gc_flag = garbage_collect;
	struct garbage_frame* gframe = &garbage_collector->frame_stack[garbage_collector->frames - 1];
	if(!noreg_head)
		gframe->to_collect[gframe->collect_values++] = value;
	
	if (value->type == value_type_object) {
		unsigned long size = 0;
		const struct value** children = get_children(&value->payload.object,&size);
		for (unsigned long i = 0; i < size; i++)
			gc_register_value(garbage_collector, children[i], 0);
	}
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

void trace_value(struct value* value) {
	if (value->gc_flag == garbage_keep)
		return;
	value->gc_flag = garbage_keep;
	
	if (value->type == value_type_object) {
		unsigned long size = 0;
		const struct value** children = get_children(&value->payload.object, &size);
		for (unsigned long i = 0; i < size; i++)
			trace_value(children[i]);
	}
}

void gc_collect(struct garbage_collector* garbage_collector) {
	struct garbage_frame* top = &garbage_collector->frame_stack[garbage_collector->frames - 1];
	for (unsigned long i = 0; i < top->collect_values; i++)
		if (top->to_collect[i]->gc_flag != garbage_trace)
			top->to_collect[i]->gc_flag = garbage_collect;
	for (unsigned long i = 0; i < top->trace_values; i++)
		trace_value(top->to_trace[i]);
	for (unsigned long i = 0; i < top->collect_values; i++) {
		if (top->to_collect[i]->gc_flag == garbage_collect || garbage_collector->frames < 2) {
			free_value(top->to_collect[i]);
			free(top->to_collect[i]);
		}
		else if (top->to_collect[i]->gc_flag == garbage_keep) {
			struct garbage_frame* prev = &garbage_collector->frame_stack[garbage_collector->frames - 2];
			prev->to_collect[prev->collect_values++] = top->to_collect[i];
		}
	}
	garbage_collector->frames--;
}

//void gc_protect(struct value* value) {
//	if (value->gc_flag == garbage_protected)
//		return;
//	if(value->gc_flag != garbage_uninit)
//		value->gc_flag = garbage_protected;
//	if (value->type == value_type_object) {
//		unsigned long size = 0;
//		const struct value** children = get_children(&value->payload.object, &size);
//		for (unsigned long i = 0; i < size; i++)
//			gc_protect(children[i]);
//	}
//}

//void reset_flags(struct garbage_frame* garbage_frame) {
//	for (unsigned long i = 0; i < garbage_frame->collect_values; i++)
//		if (garbage_frame->to_collect[i]->gc_flag != garbage_protected)
//			garbage_frame->to_collect[i]->gc_flag = garbage_collect;
//}

//
//void trace_frame(struct garbage_frame* garbage_frame) {
//	for (unsigned long i = 0; i < garbage_frame->collect_values; i++)
//		trace_value(garbage_frame->to_collect[i]);
//}

//void gc_collect(struct garbage_collector* garbage_collector) {
//	reset_flags(&garbage_collector->frame_stack[garbage_collector->frames - 1]);
//	if (garbage_collector->frames > 1) {
//		reset_flags(&garbage_collector->frame_stack[garbage_collector->frames - 2]); 
//		trace_frame(&garbage_collector->frame_stack[garbage_collector->frames - 2]); 
//	}
//	for (unsigned long i = 0; i < garbage_collector->frame_stack[garbage_collector->frames - 1].collect_values; i++)
//		if (garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]->gc_flag == garbage_collect) {
//			free_value(garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]);
//			free(garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]);
//		}
//		else { //transfer to prev garbage frame
//			if (garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]->gc_flag == garbage_protected)
//				garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]->gc_flag = garbage_collect;
//			if (garbage_collector->frames > 1) {
//				garbage_collector->frame_stack[garbage_collector->frames - 2].to_collect[garbage_collector->frame_stack[garbage_collector->frames - 2].collect_values++] = garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i];
//			}
//			else {
//				free_value(garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]);
//				free(garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]);
//			}
//		}
//	garbage_collector->frames--;
//}