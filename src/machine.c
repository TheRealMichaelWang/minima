#include <stdlib.h>
#include <string.h>
#include "include/error.h"
#include "include/runtime/builtins/builtins.h"
#include "include/runtime/opcodes.h"
#include "include/runtime/machine.h"

const int init_machine(struct machine* machine) {
	ERROR_ALLOC_CHECK(machine->position_stack = malloc(MACHINE_MAX_POSITIONS * sizeof(uint64_t)));
	ERROR_ALLOC_CHECK(machine->position_flags = malloc(MACHINE_MAX_POSITIONS * sizeof(uint8_t)));
	ERROR_ALLOC_CHECK(machine->var_stack = malloc(MACHINE_MAX_CALLS * sizeof(struct var_context)));
	ERROR_ALLOC_CHECK(machine->evaluation_stack = malloc(MACHINE_MAX_EVALS * sizeof(struct value*)));
	ERROR_ALLOC_CHECK(machine->constant_stack = malloc(MACHINE_MAX_EVALS * sizeof(struct value)));

	machine->evals = 0;
	machine->constants = 0;
	machine->positions = 0;
	machine->std_flag = 0;
	machine->call_size = 0;

	ERROR_ALLOC_CHECK(init_gcollect(&machine->garbage_collector));
	init_global_cache(&machine->global_cache);
	
	cache_declare_builtin(&machine->global_cache, 210724587794, builtin_print);
	cache_declare_builtin(&machine->global_cache, 6953911397310, builtin_print_line);
	cache_declare_builtin(&machine->global_cache, 6954037470346, builtin_system_cmd);
	cache_declare_builtin(&machine->global_cache, 6953969676070, builtin_random);
	cache_declare_builtin(&machine->global_cache, 210716150453, builtin_get_input);
	cache_declare_builtin(&machine->global_cache, 193498052, builtin_get_length);
	cache_declare_builtin(&machine->global_cache, 6385287881, builtin_get_hash);
	cache_declare_builtin(&machine->global_cache, 193506174, builtin_to_str);
	cache_declare_builtin(&machine->global_cache, 193500757, builtin_to_num);
	cache_declare_builtin(&machine->global_cache, 6954076481916, builtin_get_type);
	cache_declare_builtin(&machine->global_cache, 8246457940939440931, builtin_implements);
	cache_declare_builtin(&machine->global_cache, 193485979, builtin_abs);
	cache_declare_builtin(&machine->global_cache, 6385112226, builtin_ceil);
	cache_declare_builtin(&machine->global_cache, 210712519527, builtin_floor);
	return 1;
}

void machine_reset(struct machine* machine) {
	machine->evals = 0;
	machine->constants = 0;
	while (machine->call_size > 1)
		free_var_context(&machine->var_stack[--machine->call_size]);
}

void free_machine(struct machine* machine) {
	machine_reset(machine);
	if (machine->call_size > 0)
		free_var_context(&machine->var_stack[--machine->call_size]);
	
	free(machine->position_stack);
	free(machine->position_flags);
	free(machine->var_stack);
	free(machine->evaluation_stack);
	free(machine->constant_stack);

	free_gcollect(&machine->garbage_collector);
	free_global_cache(&machine->global_cache);
}

struct value* machine_pop_eval(struct machine* machine) {
	if (!machine->evals)
		return NULL;

	struct value* top = machine->evaluation_stack[--machine->evals];

	if (top->gc_flag == GARBAGE_CONSTANT) {
		if (!machine->constants)
			return NULL;

		machine->constants--;
		if (top->type == VALUE_TYPE_OBJ) {
			uint64_t children_count;
			struct value**children = object_get_children(&top->payload.object, &children_count);

			while (children_count--)
				if (children[children_count]->gc_flag == GARBAGE_CONSTANT)
					machine_pop_eval(machine);
		}
	}
	return top;
}

struct value* machine_push_eval(struct machine* machine, struct value* value)
{
	if (machine->evals == MACHINE_MAX_EVALS)
		return NULL;
	if (value->gc_flag == GARBAGE_CONSTANT)
		return machine_push_const(machine, *value, 1);
	else
		return machine->evaluation_stack[machine->evals++] = value;
}

struct value* machine_push_const(struct machine* machine, struct value const_value, int push_obj_children) {
	if (machine->evals == MACHINE_MAX_EVALS)
		return NULL;

	if (push_obj_children && const_value.type == VALUE_TYPE_OBJ) {
		uint64_t children_count;
		struct value** children = object_get_children(&const_value.payload.object, &children_count);

		for (uint_fast64_t i = 0; i < children_count; i++)
			if (children[i]->gc_flag == GARBAGE_CONSTANT)
				if(!(children[i] = machine_push_const(machine, *children[i], 1)))
					return NULL;
	}
	machine->constant_stack[machine->constants] = const_value;
	machine->constant_stack[machine->constants].gc_flag = GARBAGE_CONSTANT;
	return machine->evaluation_stack[machine->evals++] = &machine->constant_stack[machine->constants++];
}

const int machine_condition_check(struct machine* machine) {
	struct value*valptr = machine_pop_eval(machine);
	if (!valptr)
		return NULL;

	int cond = 1;
	if (valptr->type == VALUE_TYPE_NULL || (valptr->type == VALUE_TYPE_NUM && valptr->payload.numerical == 0))
		cond = 0;

	return cond;
}

const enum error machine_execute(struct machine* machine, struct chunk* chunk) {
	while (chunk->last_code != MACHINE_END)
	{
		if (!handle_opcode(chunk->last_code, machine, chunk))
			return machine->last_err;
		chunk_read_opcode(chunk);
	}
	return ERROR_SUCCESS;
}