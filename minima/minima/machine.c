#include <stdlib.h>
#include <string.h>
#include "operators.h"
#include "error.h"
#include "machine.h"

#define MAX_POSITIONS 2000
#define MAX_EVALS 256
#define MAX_CALLS 1000

#define EVAL_FLAG_RAW 0
#define EVAL_FLAG_RAW_PTR 1
#define EVAL_FLAG_VAR 2
#define EVAL_FLAG_REF 3

#define POS_NOFLAG 0
#define POS_GOTO 1

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

	machine->binary_operators[OPERATOR_EQUALS] = equals;
	machine->binary_operators[OPERATOR_NOT_EQUAL] = not_equals;
	machine->binary_operators[OPERATOR_MORE] = more;
	machine->binary_operators[OPERATOR_LESS] = less;
	machine->binary_operators[OPERATOR_MORE_EQUAL] = more_equal;
	machine->binary_operators[OPERATOR_LESS] = less_equal;

	machine->binary_operators[OPERATOR_ADD] = add;
	machine->binary_operators[OPERATOR_SUBTRACT] = subtract;
	machine->binary_operators[OPERATOR_MULTIPLY] = multiply;
	machine->binary_operators[OPERATOR_DIVIDE] = divide;
	machine->binary_operators[OPERATOR_MODULO] = modulo;

	init_gcollect(&machine->garbage_collector);
	init_label_cache(&machine->label_cache);
}

void free_machine(struct machine* machine) {
	while (machine->evals) {
		if (machine->eval_flags[machine->evals - 1] <= EVAL_FLAG_RAW_PTR)
			free_value(machine->evaluation_stack[machine->evals - 1]);
		if (machine->eval_flags[machine->evals - 1] == EVAL_FLAG_RAW_PTR)
			free(machine->evaluation_stack[machine->evals - 1]);
		machine->evals--;
	}

	while (machine->call_size > 0)
		free_var_context(&machine->var_stack[--machine->call_size]);
	
	free(machine->position_stack);
	free(machine->position_flags);
	free(machine->evaluation_stack);
	free(machine->eval_flags);
	free(machine->var_stack);

	free_gcollect(&machine->garbage_collector);
	free_label_cache(&machine->label_cache);
}

void skip_instruction(struct chunk* chunk) {
	char op_code = chunk->last_code;
	read(chunk);
	switch (op_code)
	{
	case MACHINE_LABEL:
	case MACHINE_GOTO:
	case MACHINE_STORE_VAR:
	case MACHINE_LOAD_VAR:
		read_size(chunk, sizeof(unsigned long));
		break;
	case MACHINE_LOAD_CONST:
		read_size(chunk, sizeof(struct value));
		break;
	case MACHINE_EVAL_BIN_OP:
	case MACHINE_EVAL_UNI_OP:
		read(chunk);
		break;
	}
}

void skip(struct chunk* chunk, unsigned long depth) {
	while (chunk->last_code != 0)
	{
		char op_code = chunk->last_code;
		if (op_code == MACHINE_SKIP || op_code == MACHINE_FLAG_SKIP || op_code == MACHINE_COND_SKIP || op_code == MACHINE_LABEL)
			depth++;
		else if (op_code == MACHINE_END_SKIP)
			depth--;
		if (depth == 0)
			break;
		skip_instruction(chunk);
	}
}

int condition_check(struct machine* machine) {
	if (machine->evals == 0)
		return 0;
	struct value* valptr = machine->evaluation_stack[--machine->evals];
	int cond = 1;
	if (valptr->type == VALUE_TYPE_NULL || (valptr->type == VALUE_TYPE_NUM && as_double(valptr)) == 0)
		cond = 0;
	if (machine->eval_flags[machine->evals] <= EVAL_FLAG_RAW_PTR)
		free_value(machine->evaluation_stack[machine->evals]);
	if (machine->eval_flags[machine->evals] == EVAL_FLAG_RAW_PTR)
		free(machine->evaluation_stack[machine->evals]);
	return cond;
}

int execute(struct machine* machine, struct chunk* chunk) {
	while (!end(chunk))
	{
		switch (read(chunk))
		{
		case MACHINE_LOAD_VAR:
			machine->evaluation_stack[machine->evals] = retrieve_var(&machine->var_stack[machine->call_size - 1], read_ulong(chunk));
			machine->eval_flags[machine->evals++] = EVAL_FLAG_VAR;
			break;
		case MACHINE_LOAD_CONST:
			machine->evaluation_stack[machine->evals] = read_size(chunk, sizeof(struct value));
			machine->eval_flags[machine->evals++] = EVAL_FLAG_RAW;
			break;
		case MACHINE_STORE_VAR: {
			if (machine->evals == 0)
				return ERROR_INSUFFICIENT_EVALS;
			struct value* valptr = machine->evaluation_stack[--machine->evals];
			if (machine->eval_flags[machine->evals] != EVAL_FLAG_RAW_PTR && machine->eval_flags[machine->evals] != EVAL_FLAG_REF) {
				struct value* oldptr = valptr;
				valptr = malloc(sizeof(struct value));
				if (valptr == NULL)
					return ERROR_INSUFFICIENT_MEMORY;
				if (!machine->eval_flags[machine->evals])
					memcpy(valptr, oldptr, sizeof(struct value));
				else
					if (!copy_value(valptr, oldptr))
						return ERROR_INSUFFICIENT_MEMORY;
				register_value(&machine->garbage_collector, valptr);
			}
			if (!emplace_var(&machine->var_stack[machine->call_size - 1], read_ulong(chunk), valptr))
				return ERROR_INSUFFICIENT_MEMORY;
			break;
		}
		case MACHINE_EVAL_BIN_OP: {
			if (machine->evals < 2)
				return ERROR_INSUFFICIENT_EVALS;
			struct value* value_a = machine->evaluation_stack[--machine->evals];
			char flag_a = machine->eval_flags[machine->evals];
			struct value* value_b = machine->evaluation_stack[--machine->evals];
			char flag_b = machine->eval_flags[machine->evals];
			char bin_op = read(chunk);
			struct value* result = (*machine->binary_operators[bin_op])(value_a, value_b);
			if (flag_a <= EVAL_FLAG_RAW_PTR)
				free_value(value_a);
			if (flag_a == EVAL_FLAG_RAW_PTR)
				free(value_a);
			if (flag_b <= EVAL_FLAG_RAW_PTR)
				free_value(value_b);
			if (flag_b == EVAL_FLAG_RAW_PTR)
				free(value_b);
			machine->evaluation_stack[machine->evals] = result;
			machine->eval_flags[machine->evals++] = EVAL_FLAG_RAW_PTR;
			break;
		}
		case MACHINE_SKIP:
			skip(chunk, 0);
			break;
		case MACHINE_MARK:
			machine->position_stack[machine->positions] = chunk->pos - 1;
			machine->position_flags[machine->positions++] = POS_NOFLAG;
			break;
		case MACHINE_RETURN:
			jump_to(machine, machine->position_stack[--machine->positions]);
			break;
		case MACHINE_GOTO:
			machine->position_stack[machine->positions] = chunk->pos + sizeof(unsigned long);
			machine->position_flags[machine->positions++] = POS_GOTO;

			jump_to(chunk, retrieve_pos(&machine->label_cache, read_ulong(chunk)));
			break;
		case MACHINE_RETURN_GOTO:
			while (machine->position_flags[machine->positions - 1] != POS_GOTO)
				machine->positions--;
			jump_to(chunk, machine->position_stack[--machine->positions]);
			break;
		case MACHINE_LABEL:
			if (!insert_label(&machine->label_cache, read_ulong(chunk), chunk->pos + sizeof(unsigned long)))
				return ERROR_LABEL_REDEFINE;
			read(chunk);
			skip(chunk, 1);
			break;
		case MACHINE_COND_SKIP:
			if (!condition_check(machine))
				skip(chunk, 0);
			break;
		case MACHINE_COND_RETURN:
			if (!condition_check(machine))
				jump_to(machine, machine->position_stack[--machine->positions]);
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
				return ERROR_INSUFFICIENT_MEMORY;
			break;
		case MACHINE_CLEAN:
			if (machine->call_size == 0)
				return ERROR_INSUFFICIENT_CALLS;
			free_var_context(&machine->var_stack[--machine->call_size]);
			break;
		}
	}
	return 0;
}