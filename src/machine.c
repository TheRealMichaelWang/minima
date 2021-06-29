#include <stdlib.h>
#include <string.h>
#include "include/error.h"
#include "include/runtime/builtins/builtins.h"
#include "include/runtime/opcodes.h"
#include "include/runtime/machine.h"

const int init_machine(struct machine* machine) {
	ERROR_ALLOC_CHECK(machine->position_stack = malloc(MACHINE_MAX_POSITIONS * sizeof(uint64_t)));
	ERROR_ALLOC_CHECK(machine->position_flags = malloc(MACHINE_MAX_POSITIONS * sizeof(char)));
	ERROR_ALLOC_CHECK(machine->evaluation_stack = malloc(MACHINE_MAX_EVALS * sizeof(struct value)));
	ERROR_ALLOC_CHECK(machine->eval_flags = malloc(MACHINE_MAX_EVALS * sizeof(char)));
	ERROR_ALLOC_CHECK(machine->var_stack = malloc(MACHINE_MAX_CALLS * sizeof(struct var_context)));

	machine->evals = 0;
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
	return 1;
}

void machine_reset(struct machine* machine) {
	while (machine->evals) {
		if (!machine->eval_flags[machine->evals - 1]) {
			free_value(machine->evaluation_stack[machine->evals - 1]);
			free(machine->evaluation_stack[machine->evals - 1]);
		}
		machine->evals--;
	}
	while (machine->call_size > 1)
		free_var_context(&machine->var_stack[--machine->call_size]);
}

void free_machine(struct machine* machine) {
	machine_reset(machine);
	if (machine->call_size > 0)
		free_var_context(&machine->var_stack[--machine->call_size]);
	
	free(machine->position_stack);
	free(machine->position_flags);
	free(machine->evaluation_stack);
	free(machine->eval_flags);
	free(machine->var_stack);

	free_gcollect(&machine->garbage_collector);
	free_global_cache(&machine->global_cache);
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