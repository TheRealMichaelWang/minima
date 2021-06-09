#include <stdlib.h>
#include <string.h>
#include "operators.h"
#include "error.h"
#include "io.h"
#include "machine.h"

#define MAX_POSITIONS 2000
#define MAX_EVALS 256
#define MAX_CALLS 1000

#define EVAL_FLAG_CPY 0
#define EVAL_FLAG_REF 1

struct value* (*binary_operators[14])(struct value* a, struct value* b) = {
	op_equals,
	op_not_equals,
	op_more,
	op_less,
	op_more_equal,
	op_less_equal,
	op_and,
	op_or,
	op_add,
	op_subtract,
	op_multiply,
	op_divide,
	op_modulo,
	op_power
};

struct value* (*unary_operators[6])(struct value* a) = {
	op_copy,
	op_invert,
	op_negate,
	op_alloc,
	op_increment,
	op_decriment
};

void init_machine(struct machine* machine) {
	machine->position_stack = malloc(MAX_POSITIONS * sizeof(unsigned long));
	machine->position_flags = malloc(MAX_POSITIONS * sizeof(char));
	machine->evaluation_stack = malloc(MAX_EVALS * sizeof(struct value));
	machine->eval_flags = malloc(MAX_EVALS * sizeof(char));
	machine->var_stack = malloc(MAX_CALLS * sizeof(struct var_context));

	machine->evals = 0;
	machine->positions = 0;
	machine->std_flag = 0;
	machine->call_size = 0;

	init_gcollect(&machine->garbage_collector);
	init_label_cache(&machine->label_cache);
	init_builtin_register(&machine->builtin_register);

	declare_builtin_proc(&machine->builtin_register, 271190290, print);
}

void reset_stack(struct machine* machine) {
	while (machine->evals) {
		if (machine->eval_flags[machine->evals - 1] == EVAL_FLAG_CPY) {
			free_value(machine->evaluation_stack[machine->evals - 1]);
			free(machine->evaluation_stack[machine->evals - 1]);
		}
		machine->evals--;
	}
	while (machine->call_size > 1)
		free_var_context(&machine->var_stack[--machine->call_size]);
}

void free_machine(struct machine* machine) {
	reset_stack(machine);
	if (machine->call_size > 0)
		free_var_context(&machine->var_stack[--machine->call_size]);
	
	free(machine->position_stack);
	free(machine->position_flags);
	free(machine->evaluation_stack);
	free(machine->eval_flags);
	free(machine->var_stack);

	free_gcollect(&machine->garbage_collector);
	free_label_cache(&machine->label_cache);
	free_builtin_register(&machine->builtin_register);
}

inline void push_eval(struct machine* machine, struct value* value, char flags) {
	machine->evaluation_stack[machine->evals] = value;
	machine->eval_flags[machine->evals++] = flags;
	if (flags == EVAL_FLAG_CPY)
		value->gc_flag = garbage_uninit;
}

inline void free_eval(struct value* value, char flags) {
	if (flags == EVAL_FLAG_CPY) {
		free_value(value);
		free(value);
	}
}

int condition_check(struct machine* machine) {
	if (machine->evals == 0) {
		machine->last_err = error_insufficient_evals;
		return 0;
	}
	struct value* valptr = machine->evaluation_stack[--machine->evals];
	int cond = 1;
	if (valptr->type == null || (valptr->type == numerical && valptr->payload.numerical == 0))
		cond = 0;
	free_eval(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
	return cond;
}

int store_var(struct machine* machine, struct chunk* chunk) {
	if (machine->evals == 0) {
		machine->last_err = error_insufficient_evals;
		return 0;
	}
	unsigned long id = read_ulong(chunk);
	struct value* setptr = machine->evaluation_stack[--machine->evals];
	if (machine->eval_flags[machine->evals] == EVAL_FLAG_CPY) {
		struct value* varptr = retrieve_var(&machine->var_stack[machine->call_size - 1], id);
		if (varptr == NULL) {
			register_value(&machine->garbage_collector, setptr, 0);
			emplace_var(&machine->var_stack[machine->call_size - 1], id, setptr);
		}
		else {
			free_value(varptr);
			memcpy(varptr, setptr, sizeof(struct value));
			free(setptr);
			register_value(&machine->garbage_collector, varptr, 1);
		}
	}
	else
		emplace_var(&machine->var_stack[machine->call_size - 1], id, setptr);
	return 1;
}

int get_index(struct machine* machine) {
	if (machine->evals < 2) {
		machine->last_err = error_insufficient_evals;
		return 0;
	}
	struct value* index_val = machine->evaluation_stack[--machine->evals];
	char index_flag = machine->eval_flags[machine->evals];
	struct value* collection_val = machine->evaluation_stack[--machine->evals];
	char collection_flag = machine->eval_flags[machine->evals];

	if (index_val->type != numerical || collection_val->type != collection) {
		machine->last_err = error_unnexpected_type;
		return 0;
	}

	if (index_val->payload.numerical < 0 || index_val->payload.numerical > collection_val->payload.collection->size) {
		machine->last_err = error_index_out_of_range;
		return 0;
	}

	struct value* toreturn = NULL;

	if (collection_flag == EVAL_FLAG_REF) {
		toreturn = collection_val->payload.collection->inner_collection[(int)index_val->payload.numerical];
		machine->eval_flags[machine->evals] = EVAL_FLAG_REF;
	}
	else if(collection_val->payload.collection->inner_collection[(int)index_val->payload.numerical]->gc_flag == garbage_uninit){
		toreturn = malloc(sizeof(struct value));
		copy_value(toreturn, collection_val->payload.collection->inner_collection[(int)index_val->payload.numerical]);
		machine->eval_flags[machine->evals] = EVAL_FLAG_CPY;
	}
	machine->evaluation_stack[machine->evals++] = toreturn;

	free_eval(index_val, index_flag);
	free_eval(collection_val, collection_flag);

	return 1;
}

int set_index(struct machine* machine) {
	if (machine->evals < 3) {
		machine->last_err = error_insufficient_evals;
		return 0;
	}
	struct value* set_val = machine->evaluation_stack[--machine->evals];
	char set_flag = machine->eval_flags[machine->evals];
	struct value* index_val = machine->evaluation_stack[--machine->evals];
	char index_flag = machine->eval_flags[machine->evals];
	struct value* collection_val = machine->evaluation_stack[--machine->evals];
	char collection_flag = machine->eval_flags[machine->evals];
	

	if (index_val->type != numerical || collection_val->type != collection) {
		machine->last_err = error_unnexpected_type;
		return 0;
	}

	if (index_val->payload.numerical < 0 || index_val->payload.numerical > collection_val->payload.collection->size) {
		machine->last_err = error_index_out_of_range;
		return 0;
	}

	if (set_flag == EVAL_FLAG_CPY) {
		struct value* item_ptr = collection_val->payload.collection->inner_collection[(int)index_val->payload.numerical];
		free_value(item_ptr);
		memcpy(item_ptr, set_val, sizeof(struct value));
		free(set_val);
	}
	else if (set_flag == EVAL_FLAG_REF) {
		if (collection_val->payload.collection->inner_collection[(int)index_val->payload.numerical]->gc_flag == garbage_uninit) {
			free_value(collection_val->payload.collection->inner_collection[(int)index_val->payload.numerical]);
			free(collection_val->payload.collection->inner_collection[(int)index_val->payload.numerical]);
		}
		collection_val->payload.collection->inner_collection[(int)index_val->payload.numerical] = set_val;
	}

	free_eval(collection_val, collection_flag);
	free_eval(index_val, index_flag);

	return 1;
}

int eval_bin_op(struct machine* machine, struct chunk* chunk) {
	if (machine->evals < 2) {
		machine->last_err = error_insufficient_evals;
		return 0;
	}
	struct value* value_b = machine->evaluation_stack[--machine->evals];
	char flag_b = machine->eval_flags[machine->evals];
	struct value* value_a = machine->evaluation_stack[--machine->evals];
	char flag_a = machine->eval_flags[machine->evals];
	char bin_op = read(chunk);
	struct value* result = (*binary_operators[bin_op])(value_a, value_b);
	free_eval(value_a, flag_a);
	free_eval(value_b, flag_b);
	push_eval(machine, result, EVAL_FLAG_CPY);
	return 1;
}

int eval_uni_op(struct machine* machine, struct chunk* chunk) {
	if (machine->evals < 1) {
		machine->last_err = error_insufficient_evals;
		return 0;
	}
	char uni_op = read(chunk);
	struct value* result = (*unary_operators[uni_op])(machine->evaluation_stack[--machine->evals]);
	if (result != machine->evaluation_stack[machine->evals])
		free_eval(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
	push_eval(machine, result, EVAL_FLAG_CPY);
	return 1;
}

int eval_builtin(struct machine* machine, struct chunk* chunk) {
	unsigned long id = read_ulong(chunk);
	unsigned long arguments = read_ulong(chunk);

	if (machine->evals < arguments) {
		machine->last_err = error_insufficient_evals;
		return 0;
	}
	struct value* result = invoke_builtin(&machine->builtin_register, id, &machine->evaluation_stack[machine->evals - arguments], arguments);
	if (result == NULL) {
		machine->last_err = error_label_undefined;
		return 0;
	}
	for (unsigned long i = machine->evals - arguments; i < machine->evals; i++)
		free_eval(machine->evaluation_stack[i], machine->eval_flags[i]);
	machine->evals -= arguments;
	push_eval(machine, result, EVAL_FLAG_CPY);
}

int build_collection(struct machine* machine, struct chunk* chunk) {
	unsigned long req_size = read_ulong(chunk);
	if (machine->evals < req_size) {
		machine->last_err = error_insufficient_evals;
		return 0;
	}

	struct collection* collection = malloc(sizeof(struct collection));
	if (collection == NULL) {
		machine->last_err = error_insufficient_memory;
		return 0;
	}

	if (!init_collection(collection, req_size)) {
		machine->last_err = error_insufficient_memory;
		return 0;
	}

	while (req_size--)
		collection->inner_collection[req_size] = machine->evaluation_stack[--machine->evals];

	struct value* new_val = malloc(sizeof(struct value));
	if (new_val == NULL) {
		machine->last_err = error_insufficient_memory;
		return 0;
	}

	init_col(new_val, collection);

	push_eval(machine, new_val, EVAL_FLAG_CPY);
	return 1;
}

int execute(struct machine* machine, struct chunk* chunk) {
	while (!end_chunk(chunk))
	{
		switch (read(chunk))
		{
		case MACHINE_LOAD_VAR: {
			struct value* var_ptr = retrieve_var(&machine->var_stack[machine->call_size - 1], read_ulong(chunk));
			if (!var_ptr) 
				return error_variable_undefined;
			push_eval(machine, var_ptr, EVAL_FLAG_REF);
			break; 
		}
		case MACHINE_LOAD_CONST: {
			struct value* to_push = malloc(sizeof(struct value));
			if (to_push == NULL)
				return error_insufficient_memory;
			copy_value(to_push, read_size(chunk, sizeof(struct value)));
			push_eval(machine, to_push, EVAL_FLAG_CPY);
			break;
		}
		case MACHINE_STORE_VAR: {
			if (!store_var(machine, chunk))
				return machine->last_err;
			break;
		}
		case MACHINE_EVAL_BIN_OP: {
			if(!eval_bin_op(machine, chunk))
				return machine->last_err;
			break;
		}
		case MACHINE_EVAL_UNI_OP: {
			if (!eval_uni_op(machine, chunk))
				return machine->last_err;
			break;
		}
		case MACHINE_SKIP:
			skip(chunk, 0);
			break;
		case MACHINE_MARK:
			machine->position_stack[machine->positions] = chunk->pos - 1;
			machine->position_flags[machine->positions++] = 0;
			break;
		case MACHINE_GOTO:
			machine->position_stack[machine->positions] = chunk->pos + sizeof(unsigned long);
			machine->position_flags[machine->positions++] = 1;
			jump_to(chunk, retrieve_pos(&machine->label_cache, read_ulong(chunk)));
			break;
		case MACHINE_RETURN_GOTO:
			while (!machine->position_flags[machine->positions - 1])
				machine->positions--;
			jump_to(chunk, machine->position_stack[--machine->positions]);
			break;
		case MACHINE_LABEL: {
			unsigned long id = read_ulong(chunk);
			if (!insert_label(&machine->label_cache, id, chunk->pos /*+ sizeof(unsigned long)*/))
				return error_label_redefine;
			read(chunk);
			skip(chunk, 1);
			break; 
		}
		case MACHINE_COND_SKIP:
			if (!condition_check(machine))
				skip(chunk, 0);
			break;
		case MACHINE_COND_RETURN:
			--machine->positions;
			if (condition_check(machine))
				jump_to(chunk, machine->position_stack[machine->positions]);
			break;
		case MACHINE_FLAG:
			machine->std_flag = 1;
			break;
		case MACHINE_RESET_FLAG:
			machine->std_flag = 0;
			break;
		case MACHINE_FLAG_SKIP:
			if (machine->std_flag)
				skip(chunk, 0);
			break;
		case MACHINE_NEW_FRAME:
			if (!init_var_context(&machine->var_stack[machine->call_size++], &machine->garbage_collector))
				return error_insufficient_memory;
			break;
		case MACHINE_CLEAN:
			if (machine->call_size == 0)
				return machine->last_err = error_insufficient_calls;
			free_var_context(&machine->var_stack[--machine->call_size]);
			break;
		case MACHINE_BUILD_COL: {
			if (!build_collection(machine, chunk))
				return machine->last_err;
			break;
		}
		case MACHINE_GET_INDEX:
			if(!get_index(machine))
				return machine->last_err;
			break;
		case MACHINE_SET_INDEX:
			if (!set_index(machine))
				return machine->last_err;
			break;
		case MACHINE_PROTECT:
			gc_protect(machine->evaluation_stack[machine->evals - 1]);
			break;
		case MACHINE_POP:
			free_eval(machine->evaluation_stack[--machine->evals], machine->eval_flags[machine->evals]);
			break;
		case MACHINE_CALL_EXTERN:
			eval_builtin(machine, chunk);
			break;
		}
	}
	return 0;
}