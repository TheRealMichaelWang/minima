#include <stdlib.h>
#include "garbage.h"

#define MAX_GARBAGE 1000
#define MAX_VALUES 1000

void init_gcollect(struct garbage_collector* garbage_collector) {
	garbage_collector->frame_stack = malloc(sizeof(struct garbage_frame) * MAX_GARBAGE);
	garbage_collector->frames = 0;
}

void free_gcollect(struct garbage_collector* garbage_collector) {
	while (garbage_collector->frames > 0)
		gc_collect(garbage_collector);
	free(garbage_collector->frame_stack);
}

void init_gframe(struct garbage_frame* garbage_frame) {
	garbage_frame->to_collect = malloc(sizeof(struct value*) * MAX_VALUES);
	garbage_frame->values = 0;
}

void free_gframe(struct garbage_frame* garbgage_frame) {
	free(garbgage_frame->to_collect);
}

void register_value(struct garbage_collector* garbage_collector, struct value* value, int noreg_head) {
	if (value->gc_flag != garbage_uninit)
		return;
	value->gc_flag = garbage_collect;
	struct garbage_frame* gframe = &garbage_collector->frame_stack[garbage_collector->frames - 1];
	if(!noreg_head)
		gframe->to_collect[gframe->values++] = value;
	
	if (value->type == collection) {
		for (unsigned long i = 0; i < value->payload.collection->size; i++)
			register_value(garbage_collector, value->payload.collection->inner_collection[i], 0);
	}
}

void trace_value(struct value* value) {
	if (value->gc_flag == garbage_keep || value->gc_flag == garbage_protected)
		return;
	value->gc_flag = garbage_keep;
	
	if (value->type == collection) {
		for (unsigned long i = 0; i < value->payload.collection->size; i++)
			trace_value(value->payload.collection->inner_collection[i]);
	}
}

void gc_protect(struct value* value) {
	if (value->gc_flag == garbage_protected)
		return;
	if(value->gc_flag != garbage_uninit)
		value->gc_flag = garbage_protected;
	if (value->type == collection) {
		for (unsigned long i = 0; i < value->payload.collection->size; i++)
			gc_protect(value->payload.collection->inner_collection[i]);
	}
}

void reset_flags(struct garbage_frame* garbage_frame) {
	for (unsigned long i = 0; i < garbage_frame->values; i++)
		if (garbage_frame->to_collect[i]->gc_flag != garbage_protected)
			garbage_frame->to_collect[i]->gc_flag = garbage_collect;
}

void trace_frame(struct garbage_frame* garbage_frame) {
	for (unsigned long i = 0; i < garbage_frame->values; i++)
		trace_value(garbage_frame->to_collect[i]);
}

void gc_collect(struct garbage_collector* garbage_collector) {
	reset_flags(&garbage_collector->frame_stack[garbage_collector->frames - 1]);
	if (garbage_collector->frames > 1)
		trace_frame(&garbage_collector->frame_stack[garbage_collector->frames - 2]);
	for (unsigned long i = 0; i < garbage_collector->frame_stack[garbage_collector->frames - 1].values; i++)
		if (garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]->gc_flag == garbage_collect) {
			free_value(garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]);
			free(garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]);
		}
		else { //transfer to prev garbage frame
			if (garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]->gc_flag == garbage_protected)
				garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]->gc_flag = garbage_collect;
			if (garbage_collector->frames > 1) {
				garbage_collector->frame_stack[garbage_collector->frames - 2].to_collect[garbage_collector->frame_stack[garbage_collector->frames - 2].values++] = garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i];
			}
			else {
				free_value(garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]);
				free(garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]);
			}
		}
	free_gframe(&garbage_collector->frame_stack[garbage_collector->frames - 1]);
	garbage_collector->frames--;
}