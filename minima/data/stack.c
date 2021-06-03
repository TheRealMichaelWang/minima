#include <stdlib.h>
#include <string.h>
#include "stack.h"

void init_stack(struct stack* stack, const unsigned int elem_size, const unsigned long alloc_elems) {
	stack->elem_size = elem_size;
	stack->alloc_elems = alloc_elems;
	stack->size = 0;
	stack->data = malloc(elem_size * alloc_elems);
}

void free_stack(struct stack* stack) {
	free(stack->data);
}

void push(struct stack* stack, const void* element) {
	if (stack->size == stack->alloc_elems)
		exit(1); //max allocated size reached
	memcpy(&stack->data[stack->size * stack->elem_size], element, stack->elem_size);
	stack->size++;
}

const void* pop(struct stack* stack) {
	if (stack->size == 0)
		exit(1);
	const void* last_pos = &stack->data[(stack->size - 1) * stack->elem_size];
	stack->size--;
	return last_pos;
}