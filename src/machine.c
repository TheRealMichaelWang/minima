#include <stdlib.h>
#include <string.h>
#include "include/error.h"
#include "include/runtime/builtins/builtins.h"
#include "include/runtime/opcodes.h"
#include "include/runtime/machine.h"

#define STACK_CHECK if(machine->evals == MACHINE_MAX_EVALS || machine->constants == MACHINE_MAX_EVALS || machine->positions == MACHINE_MAX_POSITIONS || machine->call_size == MACHINE_MAX_CALLS) { machine->last_err = ERROR_STACK_OVERFLOW; return 0; }
#define MATCH_EVALS(MIN_EVALS) if(machine->evals < MIN_EVALS) { machine->last_err = ERROR_INSUFFICIENT_EVALS; return 0; }
#define NULL_CHECK(PTR, ERROR) if(PTR == NULL) { machine->last_err = ERROR; return 0; }

const int init_machine(struct machine* machine) {
	ERROR_ALLOC_CHECK(machine->position_stack = malloc(MACHINE_MAX_POSITIONS * sizeof(uint64_t)));
	ERROR_ALLOC_CHECK(machine->position_flags = malloc(MACHINE_MAX_POSITIONS * sizeof(char)));
	ERROR_ALLOC_CHECK(machine->var_stack = malloc(MACHINE_MAX_CALLS * sizeof(struct var_context)));
	ERROR_ALLOC_CHECK(machine->evaluation_stack = malloc(MACHINE_MAX_EVALS * sizeof(struct value*)));
	ERROR_ALLOC_CHECK(machine->constant_stack = malloc(MACHINE_MAX_EVALS * sizeof(struct value)));

	machine->evals = 0;
	machine->constants = 0;
	machine->positions = 0;
	machine->std_flag = 0;
	machine->call_size = 0;

	init_gcollect(&machine->garbage_collector);
	init_global_cache(&machine->global_cache);

	cache_declare_builtin(&machine->global_cache, 271190290, builtin_print);
	cache_declare_builtin(&machine->global_cache, 359345086, builtin_print_line);
	cache_declare_builtin(&machine->global_cache, 485418122, builtin_system_cmd);
	cache_declare_builtin(&machine->global_cache, 417623846, builtin_random);
	cache_declare_builtin(&machine->global_cache, 262752949, builtin_get_input);
	cache_declare_builtin(&machine->global_cache, 193498052, builtin_get_length);
	cache_declare_builtin(&machine->global_cache, 2090320585, builtin_get_hash);
	cache_declare_builtin(&machine->global_cache, 193506174, builtin_to_str);
	cache_declare_builtin(&machine->global_cache, 193500757, builtin_to_num);
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

const struct value* pop_eval(struct machine* machine) {
	if (!machine->evals)
		return NULL;

	struct value* top = machine->evaluation_stack[--machine->evals];

	if (top->gc_flag == GARBAGE_UNINIT) {
		if (!machine->constants)
			return NULL;

		machine->constants--;
		if (top->type == VALUE_TYPE_OBJ) {
			uint64_t children_count;
			struct value** children = object_get_children(&top->payload.object, &children_count);

			while (children_count--)
				if (children[children_count]->gc_flag == GARBAGE_UNINIT)
					pop_eval(machine);
		}
	}
	return top;
}

const struct value* push_eval(struct machine* machine, struct value* value, int push_obj_children)
{
	STACK_CHECK;

	if (value->gc_flag == GARBAGE_UNINIT) {
		if (value->type == VALUE_TYPE_OBJ && push_obj_children) {
			uint64_t children_count;
			struct value** children = object_get_children(&value->payload.object, &children_count);

			for (uint_fast64_t i = 0; i < children_count; i++)
				if (children[i]->gc_flag == GARBAGE_UNINIT)
					push_eval(machine, children[i], 1);
		}

		machine->constant_stack[machine->constants] = *value;
		value = &machine->constant_stack[machine->constants++];
	}

	machine->evaluation_stack[machine->evals++] = value;
	return value;
}

const int condition_check(struct machine* machine) {
	MATCH_EVALS(1);

	struct value* valptr = pop_eval(machine);
	NULL_CHECK(valptr, ERROR_INSUFFICIENT_EVALS);

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
		chunk_read(chunk);
	}
	return ERROR_SUCCESS;
}