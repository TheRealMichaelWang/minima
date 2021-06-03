#include <stdlib.h>
#include "collection.h"
#include "garbage.h"

#define GARBAGE_COLLECT 0
#define GARBAGE_KEEP 1
#define GARBAGE_PROTECTED 2

void init_gframe(struct garbage_frame* garbage_frame) {
	garbage_frame->to_collect = malloc(sizeof(struct value*) * MAX_VALUES);
	garbage_frame->values = 0;
}

void init_gcollect(struct garbage_collector* garbage_collector) {
	garbage_collector->frame_stack = malloc(sizeof(struct garbage_frame) * MAX_GARBAGE);
	garbage_collector->frames = 0;
}

void free_gframe(struct garbage_frame* garbgage_frame) {
	free(garbgage_frame->to_collect);
}

void free_gcollect(struct garbage_collector* garbage_collector) {
	while (garbage_collector->frames > 0)
		gc_collect(garbage_collector);
	free(garbage_collector->frame_stack);
}

void trace_value(struct value* value) {
	if (value->gc_flag == GARBAGE_KEEP || value->gc_flag == GARBAGE_PROTECTED)
		return;
	value->gc_flag = GARBAGE_KEEP;
	
	if (value->type == VALUE_TYPE_COL) {
		struct collection* collection = value->ptr;
		for (unsigned long i = 0; i < collection->size; i++)
			trace_value(collection->inner_collection[i]);
	}
}

void gc_protect(struct value* value) {
	if (value->gc_flag)
		return;
	if (value->type == VALUE_TYPE_COL) {
		struct collection* collection = value->ptr;
		for (unsigned long i = 0; i < collection->size; i++)
			gc_protect(collection->inner_collection[i]);
	}
}

void reset_flags(struct garbage_frame* garbage_frame) {
	for (unsigned long i = 0; i < garbage_frame->values; i++)
		if(garbage_frame->to_collect[i]->gc_flag != GARBAGE_PROTECTED)
			garbage_frame->to_collect[i]->gc_flag = GARBAGE_COLLECT;
}

void trace_frame(struct garbage_frame* garbage_frame) {
	for (unsigned long i = 0; i < garbage_frame->values; i++)
		trace_value(garbage_frame->to_collect[i]);
}

void gc_collect(struct garbage_collector* garbage_collector) {
	reset_flags(&garbage_collector->frame_stack[garbage_collector->frames - 1]);
	if (garbage_collector->frames > 1) {
		reset_flags(&garbage_collector->frame_stack[garbage_collector->frames - 2]);
		for (unsigned long i = 0; i < garbage_collector->frame_stack[garbage_collector->frames - 2].values; i++)
			trace_value(garbage_collector->frame_stack[garbage_collector->frames - 2].to_collect[i]);
	}
	for (unsigned long i = 0; i < garbage_collector->frame_stack[garbage_collector->frames - 1].values; i++)
		if (garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]->gc_flag == GARBAGE_COLLECT) {
			free_value(garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]);
			free(garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]);
		}
		else if(garbage_collector->frames > 1) { //transfer to prev garbage frame
			if (garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]->gc_flag == GARBAGE_PROTECTED)
				garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i]->gc_flag = GARBAGE_COLLECT;
			garbage_collector->frame_stack[garbage_collector->frames - 2].to_collect[garbage_collector->frame_stack[garbage_collector->frames - 2].values++] = garbage_collector->frame_stack[garbage_collector->frames - 1].to_collect[i];
		}
	free_gframe(&garbage_collector->frame_stack[garbage_collector->frames - 1]);
	garbage_collector->frames--;
}

