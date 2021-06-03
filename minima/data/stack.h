#pragma once

#ifndef STACK_H
#define STACK_H

struct stack {
	unsigned long alloc_elems;
	unsigned long size;
	unsigned int elem_size;
	char* data;
};

void init_stack(struct stack* stack, const unsigned int elem_size, const unsigned long alloc_elems);

void free_stack(struct stack* stack);

void push(struct stack* stack, const void* element);

const void* pop(struct stack* stack);

inline const char is_empty(struct stack* stack) {
	return stack->size == 0;
}

#endif // !STACK_H
