#include <stdlib.h>
#include <string.h>
#include "operators.h"
#include "error.h"
#include "io.h"
#include "collection.h"
#include "record.h"
#include "hash.h"
#include "machine.h"

#define MAX_POSITIONS 2000
#define MAX_EVALS 2000
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
	init_global_cache(&machine->global_cache);

	declare_builtin_proc(&machine->global_cache, 271190290, print);
	declare_builtin_proc(&machine->global_cache, 262752949, get_input);
	declare_builtin_proc(&machine->global_cache, 193498052, get_length);
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
	free_global_cache(&machine->global_cache);
}

inline const int push_eval(struct machine* machine, struct value* value, char flags) {
	if (machine->evals == MAX_EVALS) {
		machine->last_err = error_stack_overflow;
		return 0;
	}
	machine->evaluation_stack[machine->evals] = value;
	machine->eval_flags[machine->evals++] = flags;
	if (flags == EVAL_FLAG_CPY)
		value->gc_flag = garbage_uninit;
	return 1;
}

inline void free_eval(struct value* value, char flags) {
	if (flags == EVAL_FLAG_CPY) {
		free_value(value);
		free(value);
	}
}

inline const int match_evals(struct machine* machine, const int min_evals) {
	if (machine->evals < min_evals) {
		machine->last_err = error_insufficient_evals;
		return 0;
	}
	return 1;
}

const int condition_check(struct machine* machine) {
	if (!match_evals(machine, 1))
		return 0;
	struct value* valptr = machine->evaluation_stack[--machine->evals];
	int cond = 1;
	if (valptr->type == value_type_null || (valptr->type == value_type_numerical && valptr->payload.numerical == 0))
		cond = 0;
	free_eval(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
	return cond;
}

const int store_var(struct machine* machine, struct chunk* chunk) {
	if (!match_evals(machine, 1))
		return 0;
	unsigned long id = read_ulong(chunk);
	struct value* setptr = machine->evaluation_stack[--machine->evals];
	if (machine->eval_flags[machine->evals] == EVAL_FLAG_CPY) {
		struct value* varptr = retrieve_var(&machine->var_stack[machine->call_size - 1], id);
		if (varptr == NULL) {
			gc_register_value(&machine->garbage_collector, setptr, 0);
			emplace_var(&machine->var_stack[machine->call_size - 1], id, setptr);
		}
		else {
			free_value(varptr);
			memcpy(varptr, setptr, sizeof(struct value));
			free(setptr);
			gc_register_value(&machine->garbage_collector, varptr, 1);
		}
	}
	else
		emplace_var(&machine->var_stack[machine->call_size - 1], id, setptr);
	return 1;
}

const int get_index(struct machine* machine) {
	if (!match_evals(machine, 2))
		return 0;

	struct value* index_val = machine->evaluation_stack[--machine->evals];
	char index_flag = machine->eval_flags[machine->evals];
	struct value* collection_val = machine->evaluation_stack[--machine->evals];
	char collection_flag = machine->eval_flags[machine->evals];

	if (index_val->type != value_type_numerical || collection_val->type != value_type_object || collection_val->payload.object.type != obj_type_collection) {
		machine->last_err = error_unnexpected_type;
		return 0;
	}

	struct collection* collection = collection_val->payload.object.ptr.collection;
	unsigned long index = index_val->payload.numerical;

	if (index > collection->size) {
		machine->last_err = error_index_out_of_range;
		return 0;
	}

	struct value* toreturn = NULL;

	if (collection->inner_collection[index]->gc_flag == garbage_uninit) {
		toreturn = malloc(sizeof(struct value));
		if (!copy_value(toreturn, collection->inner_collection[index]))
			return 0;
		machine->eval_flags[machine->evals] = EVAL_FLAG_CPY;
	}
	else {
		toreturn = collection->inner_collection[index];
		machine->eval_flags[machine->evals] = EVAL_FLAG_REF;
	}
	machine->evaluation_stack[machine->evals++] = toreturn;

	free_eval(index_val, index_flag);
	free_eval(collection_val, collection_flag);

	return 1;
}

const int set_index(struct machine* machine) {
	if (!match_evals(machine, 3))
		return 0;

	struct value* set_val = machine->evaluation_stack[--machine->evals];
	char set_flag = machine->eval_flags[machine->evals];
	struct value* index_val = machine->evaluation_stack[--machine->evals];
	char index_flag = machine->eval_flags[machine->evals];
	struct value* collection_val = machine->evaluation_stack[--machine->evals];
	char collection_flag = machine->eval_flags[machine->evals];

	if (index_val->type != value_type_numerical || collection_val->type != value_type_object || collection_val->payload.object.type != obj_type_collection) {
		machine->last_err = error_unnexpected_type;
		return 0;
	}

	struct collection* collection = collection_val->payload.object.ptr.collection;
	unsigned long index = index_val->payload.numerical;

	if (index > collection->size) {
		machine->last_err = error_index_out_of_range;
		return 0;
	}

	if (set_flag == EVAL_FLAG_CPY) {
		struct value* item_ptr = collection->inner_collection[index];
		free_value(item_ptr);
		memcpy(item_ptr, set_val, sizeof(struct value));
		free(set_val);
		if (collection_flag == EVAL_FLAG_REF)
			gc_register_value(&machine->garbage_collector, item_ptr, 1);
	}
	else if (set_flag == EVAL_FLAG_REF) {
		if (collection->inner_collection[index]->gc_flag == garbage_uninit) {
			free_value(collection->inner_collection[index]);
			free(collection->inner_collection[index]);
		}
		collection->inner_collection[index] = set_val;
	}

	free_eval(collection_val, collection_flag);
	free_eval(index_val, index_flag);

	return 1;
}

const int set_property(struct machine* machine, struct chunk* chunk) {
	if (!match_evals(machine, 2))
		return 0;

	unsigned long property = read_ulong(chunk);

	struct value* set_val = machine->evaluation_stack[--machine->evals];
	char set_flag = machine->eval_flags[machine->evals];
	struct value* record_eval = machine->evaluation_stack[--machine->evals];
	char record_flag = machine->eval_flags[machine->evals];

	if (record_eval->type != value_type_object || record_eval->payload.object.type != obj_type_record) {
		machine->last_err = error_unnexpected_type;
		return 0;
	}

	struct record* record = record_eval->payload.object.ptr.record;
	struct value* property_val = get_value_ref(record, property);
	if (!property_val) {
		machine->last_err = error_property_undefined;
		return 0;
	}

	if (set_flag == EVAL_FLAG_CPY) {
		free_value(property_val);
		memcpy(property_val, set_val, sizeof(struct value));
		free(set_val);
		if (record_flag == EVAL_FLAG_REF)
			gc_register_value(&machine->garbage_collector, property_val, 1);
	}
	else {
		if (property_val->gc_flag == garbage_uninit) {
			free_value(property_val);
			free(property_val);
		}
		if (!set_value_ref(record, property, set_val)) {
			machine->last_err = error_property_undefined;
			return 0;
		}
	}

	free_eval(record_eval, record_flag);
	return 1;
}

const int get_property(struct machine* machine, struct chunk* chunk) {
	if (!match_evals(machine, 1))
		return 0;

	struct value* record_eval = machine->evaluation_stack[--machine->evals];
	char record_flag = machine->eval_flags[machine->evals];

	if (record_eval->type != value_type_object || record_eval->payload.object.type != obj_type_record) {
		machine->last_err = error_unnexpected_type;
		return 0;
	}

	struct value* toreturn = NULL;
	struct value* property_val = get_value_ref(record_eval->payload.object.ptr.record, read_ulong(chunk));
	if (!property_val) {
		machine->last_err = error_property_undefined;
		return 0;
	}

	if (property_val->gc_flag == garbage_uninit) {
		toreturn = malloc(sizeof(struct value));
		if (!copy_value(toreturn, property_val))
			return 0;
		machine->eval_flags[machine->evals] = EVAL_FLAG_CPY;
	}
	else {
		toreturn = property_val;
		machine->eval_flags[machine->evals] = EVAL_FLAG_REF;
	}
	machine->evaluation_stack[machine->evals++] = toreturn;

	free_eval(record_eval, record_flag);
	return 1;
}

int eval_bin_op(struct machine* machine, struct chunk* chunk) {
	if (!match_evals(machine, 2))
		return 0;

	struct value* value_b = machine->evaluation_stack[--machine->evals];
	char flag_b = machine->eval_flags[machine->evals];
	struct value* value_a = machine->evaluation_stack[--machine->evals];
	char flag_a = machine->eval_flags[machine->evals];
	char bin_op = read(chunk);
	struct value* result = (*binary_operators[bin_op])(value_a, value_b);
	free_eval(value_a, flag_a);
	free_eval(value_b, flag_b);
	if (!push_eval(machine, result, EVAL_FLAG_CPY))
		return 0;
	return 1;
}

int eval_uni_op(struct machine* machine, struct chunk* chunk) {
	if (!match_evals(machine, 1))
		return 0;

	char uni_op = read(chunk);
	struct value* result = (*unary_operators[uni_op])(machine->evaluation_stack[--machine->evals]);
	if (result != machine->evaluation_stack[machine->evals]) {
		free_eval(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
		if (!push_eval(machine, result, EVAL_FLAG_CPY))
			return 0;
	}
	else
		push_eval(machine, result, machine->eval_flags[machine->evals]);
	return 1;
}

const int eval_builtin(struct machine* machine, struct chunk* chunk) {
	unsigned long id = read_ulong(chunk);
	unsigned long arguments = read_ulong(chunk);

	if (!match_evals(machine, arguments))
		return 0;

	struct value* result = invoke_builtin(&machine->global_cache, id, &machine->evaluation_stack[machine->evals - arguments], arguments);
	if (result == NULL) {
		machine->last_err = error_label_undefined;
		return 0;
	}
	for (unsigned long i = machine->evals - arguments; i < machine->evals; i++)
		free_eval(machine->evaluation_stack[i], machine->eval_flags[i]);
	machine->evals -= arguments;
	return push_eval(machine, result, EVAL_FLAG_CPY);
}

const int build_collection(struct machine* machine, struct chunk* chunk) {
	unsigned long req_size = read_ulong(chunk);
	if (!match_evals(machine, req_size))
		return 0;

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

	struct object obj;
	init_object_col(&obj, collection);
	init_obj_value(new_val, obj);

	return push_eval(machine, new_val, EVAL_FLAG_CPY);
}

const int goto_as(struct machine* machine, struct chunk* chunk) {
	if (!match_evals(machine, 1))
		return 0;

	struct value* record_eval = machine->evaluation_stack[machine->evals - 1];
	char record_flag = machine->eval_flags[machine->evals - 1];

	if (record_eval->type != value_type_object || record_eval->payload.object.type != obj_type_record) {
		machine->last_err = error_unnexpected_type;
		return 0;
	}

	if (machine->positions == MAX_POSITIONS) {
		machine->last_err = error_stack_overflow;
		return 0;
	}

	machine->position_stack[machine->positions] = chunk->pos + sizeof(unsigned long);
	machine->position_flags[machine->positions++] = 1;
	unsigned long pos = retrieve_pos(&machine->global_cache, combine_hash(read_ulong(chunk), record_eval->payload.object.ptr.record->prototype->identifier));
	if (!pos) {
		machine->last_err = error_label_undefined;
		return 0;
	}
	jump_to(chunk, pos);
	return 1;
}

const int execute(struct machine* machine, struct chunk* chunk) {
	while (!end_chunk(chunk))
	{
		switch (read(chunk))
		{
		case MACHINE_LOAD_VAR: {
			struct value* var_ptr = retrieve_var(&machine->var_stack[machine->call_size - 1], read_ulong(chunk));
			if (!var_ptr)
				return machine->last_err = error_variable_undefined;
			if (!push_eval(machine, var_ptr, EVAL_FLAG_REF))
				return error_stack_overflow;
			break;
		}
		case MACHINE_LOAD_CONST: {
			struct value* to_push = malloc(sizeof(struct value));
			if (to_push == NULL)
				return machine->last_err = error_insufficient_memory;
			copy_value(to_push, read_size(chunk, sizeof(struct value)));
			if (!push_eval(machine, to_push, EVAL_FLAG_CPY))
				return error_stack_overflow;
			break;
		}
		case MACHINE_STORE_VAR:
			if (!store_var(machine, chunk))
				return machine->last_err;
			break;
		case MACHINE_EVAL_BIN_OP:
			if (!eval_bin_op(machine, chunk))
				return machine->last_err;
			break;
		case MACHINE_EVAL_UNI_OP:
			if (!eval_uni_op(machine, chunk))
				return machine->last_err;
			break;
		case MACHINE_MARK:
			if (machine->positions == MAX_POSITIONS)
				return machine->last_err = error_stack_overflow;
			machine->position_stack[machine->positions] = chunk->pos - 1;
			machine->position_flags[machine->positions++] = 0;
			break;
		case MACHINE_GOTO: {
			if (machine->positions == MAX_POSITIONS)
				return machine->last_err = error_stack_overflow;
			machine->position_stack[machine->positions] = chunk->pos + sizeof(unsigned long);
			machine->position_flags[machine->positions++] = 1;
			unsigned long pos = retrieve_pos(&machine->global_cache, read_ulong(chunk));
			if (!pos) {
				machine->last_err = error_label_undefined;
				return 0;
			}
			jump_to(chunk, pos);
			break; 
		}
		case MACHINE_GOTO_AS:
			if (!goto_as(machine, chunk))
				return machine->last_err;
			break;
		case MACHINE_RETURN_GOTO:
			while (!machine->position_flags[machine->positions - 1])
				machine->positions--;
			jump_to(chunk, machine->position_stack[--machine->positions]);
			break;
		case MACHINE_LABEL: {
			unsigned long id = read_ulong(chunk);
			if (!insert_label(&machine->global_cache, id, chunk->pos))
				return machine->last_err = error_label_redefine;
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
			if (machine->call_size == MAX_CALLS)
				return machine->last_err = error_stack_overflow;
			if (!init_var_context(&machine->var_stack[machine->call_size++], &machine->garbage_collector))
				return error_insufficient_memory;
			break;
		case MACHINE_CLEAN:
			if (machine->call_size == 0)
				return machine->last_err = error_insufficient_calls;
			free_var_context(&machine->var_stack[--machine->call_size]);
			break;
		case MACHINE_BUILD_COL:
			if (!build_collection(machine, chunk))
				return machine->last_err;
			break;
		case MACHINE_BUILD_PROTO: {
			unsigned long id = read_ulong(chunk);
			unsigned long properties = read_ulong(chunk);
			struct record_prototype* prototype = malloc(sizeof(struct record_prototype));
			init_record_prototype(prototype, id);
			while (properties--)
				if (!append_record_property(prototype, read_ulong(chunk))) {
					machine->last_err = error_property_redefine;
					return 0;
				}
			if (!insert_prototype(&machine->global_cache, id, prototype))
				return machine->last_err = error_record_redefine;
			break;
		}
		case MACHINE_BUILD_RECORD: {
			unsigned long id = read_ulong(chunk);
			struct record* new_rec = malloc(sizeof(struct record));
			if (new_rec == NULL)
				return machine->last_err = error_insufficient_memory;
			if (!init_record_id(&machine->global_cache, id, new_rec))
				return machine->last_err = error_record_undefined;
			for (unsigned char i = 0; i < new_rec->prototype->size; i++) {
				struct value* property = malloc(sizeof(struct value));
				if(property == NULL)
					return machine->last_err = error_insufficient_memory;
				init_null_value(property);
				new_rec->properties[i] = property;
			}
			struct value* rec_val = malloc(sizeof(struct value));
			if(rec_val == NULL)
				return machine->last_err = error_insufficient_memory;
			struct object obj;
			init_object_rec(&obj, new_rec);
			init_obj_value(rec_val, obj);
			push_eval(machine, rec_val, EVAL_FLAG_CPY);
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
		case MACHINE_GET_PROPERTY:
			if (!get_property(machine, chunk))
				return machine->last_err;
			break;
		case MACHINE_SET_PROPERTY:
			if (!set_property(machine, chunk))
				return machine->last_err;
			break;
		case MACHINE_TRACE:
			if (machine->eval_flags[machine->evals - 1] == EVAL_FLAG_REF) {
				gc_register_trace(&machine->garbage_collector, machine->evaluation_stack[machine->evals - 1]);
			}
			break;
		case MACHINE_POP:
			machine->evals--;
			free_eval(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
			break;
		case MACHINE_CALL_EXTERN:
			if (!eval_builtin(machine, chunk))
				return machine->last_err;
			break;
		}
	}
	return 0;
}