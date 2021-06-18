#include <stdlib.h>
#include <string.h>
#include "operators.h"
#include "error.h"
#include "stdlib.h"
#include "collection.h"
#include "record.h"
#include "hash.h"
#include "machine.h"

#define MAX_POSITIONS 20000
#define MAX_EVALS 10000
#define MAX_CALLS 10000

#define NULL_CHECK(PTR, MACHINE, ERROR) if(PTR == NULL) { MACHINE->last_err = ERROR; return 0; }
#define STACK_CHECK(MACHINE) if(MACHINE->evals == MAX_EVALS || MACHINE->positions == MAX_POSITIONS || machine->call_size == MAX_CALLS) { machine->last_err = ERROR_STACK_OVERFLOW; return 0; }

#define MATCH_EVALS(MIN_EVALS, MACHINE) if(MACHINE->evals < MIN_EVALS) { MACHINE->last_err = ERROR_INSUFFICIENT_EVALS; return 0; }
#define FREE_EVAL(EVAL, FLAGS) if(!FLAGS) { free_value(EVAL); free(EVAL); }

#define MACHINE_ERROR(ERROR, MACHINE) MACHINE->last_err = ERROR; return 0;

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

	cache_declare_builtin(&machine->global_cache, 271190290, builtin_print);
	cache_declare_builtin(&machine->global_cache, 359345086, builtin_print_line);
	cache_declare_builtin(&machine->global_cache, 485418122, builtin_system_cmd);
	cache_declare_builtin(&machine->global_cache, 417623846, builtin_random);
	cache_declare_builtin(&machine->global_cache, 262752949, builtin_get_input);
	cache_declare_builtin(&machine->global_cache, 193498052, builtin_get_length);
}

void machine_reset_stack(struct machine* machine) {
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
	machine_reset_stack(machine);
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
	STACK_CHECK(machine);
	machine->evaluation_stack[machine->evals] = value;
	machine->eval_flags[machine->evals++] = flags;
	if (!flags)
		value->gc_flag = GARBAGE_UNINIT;
	return 1;
}

const int condition_check(struct machine* machine) {
	MATCH_EVALS(1, machine);
	struct value* valptr = machine->evaluation_stack[--machine->evals];
	int cond = 1;
	if (valptr->type == VALUE_TYPE_NULL || (valptr->type == VALUE_TYPE_NUM && valptr->payload.numerical == 0))
		cond = 0;
	FREE_EVAL(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
	return cond;
}

const int store_var(struct machine* machine, struct chunk* chunk) {
	MATCH_EVALS(1, machine);
	unsigned long id = chunk_read_ulong(chunk);
	struct value* setptr = machine->evaluation_stack[--machine->evals];
	if (machine->eval_flags[machine->evals])
		emplace_var(&machine->var_stack[machine->call_size - 1], id, setptr);
	else {
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
	return 1;
}

const int get_index(struct machine* machine) {
	MATCH_EVALS(2, machine);

	struct value* index_val = machine->evaluation_stack[--machine->evals];
	char index_flag = machine->eval_flags[machine->evals];
	struct value* collection_val = machine->evaluation_stack[--machine->evals];
	char collection_flag = machine->eval_flags[machine->evals];

	if (index_val->type != VALUE_TYPE_NUM || collection_val->type != VALUE_TYPE_OBJ || collection_val->payload.object.type != obj_type_collection)
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE, machine);

	struct collection* collection = collection_val->payload.object.ptr.collection;
	unsigned long index = index_val->payload.numerical;

	if (index > collection->size)
		MACHINE_ERROR(ERROR_INDEX_OUT_OF_RANGE, machine);

	struct value* toreturn = NULL;

	if (collection->inner_collection[index]->gc_flag == GARBAGE_UNINIT) {
		toreturn = malloc(sizeof(struct value));
		if (!copy_value(toreturn, collection->inner_collection[index]))
			return 0;
		machine->eval_flags[machine->evals] = 0;
	}
	else {
		toreturn = collection->inner_collection[index];
		machine->eval_flags[machine->evals] = 1;
	}
	machine->evaluation_stack[machine->evals++] = toreturn;

	FREE_EVAL(index_val, index_flag);
	FREE_EVAL(collection_val, collection_flag);

	return 1;
}

const int set_index(struct machine* machine) {
	MATCH_EVALS(3, machine);

	struct value* set_val = machine->evaluation_stack[--machine->evals];
	char set_flag = machine->eval_flags[machine->evals];
	struct value* index_val = machine->evaluation_stack[--machine->evals];
	char index_flag = machine->eval_flags[machine->evals];
	struct value* collection_val = machine->evaluation_stack[--machine->evals];
	char collection_flag = machine->eval_flags[machine->evals];

	if (index_val->type != VALUE_TYPE_NUM || collection_val->type != VALUE_TYPE_OBJ || collection_val->payload.object.type != obj_type_collection)
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE, machine);

	struct collection* collection = collection_val->payload.object.ptr.collection;
	unsigned long index = index_val->payload.numerical;

	if (index > collection->size)
		MACHINE_ERROR(ERROR_INDEX_OUT_OF_RANGE, machine);

	if (set_flag) {
		if (collection->inner_collection[index]->gc_flag == GARBAGE_UNINIT) {
			free_value(collection->inner_collection[index]);
			free(collection->inner_collection[index]);
		}
		collection->inner_collection[index] = set_val;
	}
	else {
		struct value* item_ptr = collection->inner_collection[index];
		free_value(item_ptr);
		memcpy(item_ptr, set_val, sizeof(struct value));
		free(set_val);
		if (collection_flag)
			gc_register_value(&machine->garbage_collector, item_ptr, 1);
	}

	FREE_EVAL(collection_val, collection_flag);
	FREE_EVAL(index_val, index_flag);

	return 1;
}

const int set_property(struct machine* machine, struct chunk* chunk) {
	MATCH_EVALS(2, machine);

	unsigned long property = chunk_read_ulong(chunk);

	struct value* set_val = machine->evaluation_stack[--machine->evals];
	char set_flag = machine->eval_flags[machine->evals];
	struct value* record_eval = machine->evaluation_stack[--machine->evals];
	char record_flag = machine->eval_flags[machine->evals];

	if (record_eval->type != VALUE_TYPE_OBJ || record_eval->payload.object.type != obj_type_record)
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE, machine);

	struct record* record = record_eval->payload.object.ptr.record;
	struct value* property_val = record_get_ref(record, property);
	if (!property_val)
		MACHINE_ERROR(ERROR_PROPERTY_UNDEFINED, machine);

	if (set_flag) {
		if (property_val->gc_flag == GARBAGE_UNINIT) {
			free_value(property_val);
			free(property_val);
		}
		if (!record_set_ref(record, property, set_val))
			MACHINE_ERROR(ERROR_PROPERTY_UNDEFINED, machine);
	}
	else {
		free_value(property_val);
		memcpy(property_val, set_val, sizeof(struct value));
		free(set_val);
		if (record_flag)
			gc_register_value(&machine->garbage_collector, property_val, 1);
	}

	FREE_EVAL(record_eval, record_flag);
	return 1;
}

const int get_property(struct machine* machine, struct chunk* chunk) {
	MATCH_EVALS(1, machine);

	struct value* record_eval = machine->evaluation_stack[--machine->evals];
	char record_flag = machine->eval_flags[machine->evals];

	if (record_eval->type != VALUE_TYPE_OBJ || record_eval->payload.object.type != obj_type_record)
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE, machine);

	struct value* toreturn = NULL;
	struct value* property_val = record_get_ref(record_eval->payload.object.ptr.record, chunk_read_ulong(chunk));
	if (!property_val)
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE, machine);

	if (property_val->gc_flag == GARBAGE_UNINIT) {
		toreturn = malloc(sizeof(struct value));
		if (!copy_value(toreturn, property_val))
			return 0;
		machine->eval_flags[machine->evals] = 0;
	}
	else {
		toreturn = property_val;
		machine->eval_flags[machine->evals] = 1;
	}
	machine->evaluation_stack[machine->evals++] = toreturn;

	FREE_EVAL(record_eval, record_flag);
	return 1;
}

int eval_bin_op(struct machine* machine, struct chunk* chunk) {
	MATCH_EVALS(2, machine);

	struct value* value_b = machine->evaluation_stack[--machine->evals];
	char flag_b = machine->eval_flags[machine->evals];
	struct value* value_a = machine->evaluation_stack[--machine->evals];
	char flag_a = machine->eval_flags[machine->evals];
	char bin_op = chunk_read(chunk);
	struct value* result = (*binary_operators[bin_op])(value_a, value_b);
	FREE_EVAL(value_a, flag_a);
	FREE_EVAL(value_b, flag_b);
	if (!push_eval(machine, result, 0))
		return 0;
	return 1;
}

int eval_uni_op(struct machine* machine, struct chunk* chunk) {
	MATCH_EVALS(1, machine);

	char uni_op = chunk_read(chunk);
	struct value* result = (*unary_operators[uni_op])(machine->evaluation_stack[--machine->evals]);
	if (result != machine->evaluation_stack[machine->evals]) {
		FREE_EVAL(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
		if (!push_eval(machine, result, 0))
			return 0;
	}
	else
		push_eval(machine, result, machine->eval_flags[machine->evals]);
	return 1;
}

const int eval_builtin(struct machine* machine, struct chunk* chunk) {
	unsigned long id = chunk_read_ulong(chunk);
	unsigned long arguments = chunk_read_ulong(chunk);

	MATCH_EVALS(arguments, machine);

	struct value* result = cache_invoke_builtin(&machine->global_cache, id, &machine->evaluation_stack[machine->evals - arguments], arguments);
	NULL_CHECK(result, machine, ERROR_LABEL_UNDEFINED);
	for (unsigned long i = machine->evals - arguments; i < machine->evals; i++)
		FREE_EVAL(machine->evaluation_stack[i], machine->eval_flags[i]);
	machine->evals -= arguments;
	return push_eval(machine, result, 0);
}

const int build_collection(struct machine* machine, struct chunk* chunk) {
	unsigned long req_size = chunk_read_ulong(chunk);
	MATCH_EVALS(req_size, machine);

	struct collection* collection = malloc(sizeof(struct collection));
	NULL_CHECK(collection, machine, ERROR_OUT_OF_MEMORY);
	NULL_CHECK(init_collection(collection, req_size), machine, ERROR_OUT_OF_MEMORY);

	while (req_size--)
		collection->inner_collection[req_size] = machine->evaluation_stack[--machine->evals];

	struct value* new_val = malloc(sizeof(struct value));
	NULL_CHECK(new_val, machine, ERROR_OUT_OF_MEMORY);

	struct object obj;
	init_object_col(&obj, collection);
	init_obj_value(new_val, obj);

	return push_eval(machine, new_val, 0);
}

const int goto_as(struct machine* machine, struct chunk* chunk) {
	MATCH_EVALS(1, machine);

	struct value* record_eval = machine->evaluation_stack[machine->evals - 1];
	char record_flag = machine->eval_flags[machine->evals - 1];

	if (record_eval->type != VALUE_TYPE_OBJ || record_eval->payload.object.type != obj_type_record)
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE, machine);

	STACK_CHECK(machine);

	machine->position_stack[machine->positions] = chunk->pos + sizeof(unsigned long);
	machine->position_flags[machine->positions++] = 1;
	unsigned long pos = cache_retrieve_pos(&machine->global_cache, combine_hash(chunk_read_ulong(chunk), record_eval->payload.object.ptr.record->prototype->identifier));
	if (!pos)
		MACHINE_ERROR(ERROR_LABEL_UNDEFINED, machine);
	chunk_jump_to(chunk, pos);
	return 1;
}

const int machine_execute(struct machine* machine, struct chunk* chunk) {
	while (chunk->last_code != MACHINE_END)
	{
		switch (chunk->last_code)
		{
		case MACHINE_LOAD_VAR: {
			struct value* var_ptr = retrieve_var(&machine->var_stack[machine->call_size - 1], chunk_read_ulong(chunk));
			if (!var_ptr)
				return machine->last_err = ERROR_VARIABLE_UNDEFINED;
			if (!push_eval(machine, var_ptr, 1))
				return ERROR_STACK_OVERFLOW;
			break;
		}
		case MACHINE_LOAD_CONST: {
			struct value* to_push = malloc(sizeof(struct value));
			if (to_push == NULL)
				return machine->last_err = ERROR_OUT_OF_MEMORY;
			copy_value(to_push, chunk_read_size(chunk, sizeof(struct value)));
			if (!push_eval(machine, to_push, 0))
				return ERROR_STACK_OVERFLOW;
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
				return machine->last_err = ERROR_STACK_OVERFLOW;
			machine->position_stack[machine->positions] = chunk->pos - 1;
			machine->position_flags[machine->positions++] = 0;
			break;
		case MACHINE_GOTO: {
			if (machine->positions == MAX_POSITIONS)
				return machine->last_err = ERROR_STACK_OVERFLOW;
			machine->position_stack[machine->positions] = chunk->pos + sizeof(unsigned long);
			machine->position_flags[machine->positions++] = 1;
			unsigned long pos = cache_retrieve_pos(&machine->global_cache, chunk_read_ulong(chunk));
			if (!pos) {
				machine->last_err = ERROR_LABEL_UNDEFINED;
				return 0;
			}
			chunk_jump_to(chunk, pos);
			break; 
		}
		case MACHINE_GOTO_AS:
			if (!goto_as(machine, chunk))
				return machine->last_err;
			break;
		case MACHINE_RETURN_GOTO:
			while (!machine->position_flags[machine->positions - 1])
				machine->positions--;
			chunk_jump_to(chunk, machine->position_stack[--machine->positions]);
			break;
		case MACHINE_LABEL: {
			unsigned long id = chunk_read_ulong(chunk);
			if (!cache_insert_label(&machine->global_cache, id, chunk->pos))
				return machine->last_err = ERROR_LABEL_REDEFINE;
			chunk_read(chunk);
			chunk_skip(chunk, 1);
			break;
		}
		case MACHINE_COND_SKIP:
			if (!condition_check(machine))
				chunk_skip(chunk, 0);
			break;
		case MACHINE_COND_RETURN:
			--machine->positions;
			if (condition_check(machine))
				chunk_jump_to(chunk, machine->position_stack[machine->positions]);
			break;
		case MACHINE_FLAG:
			machine->std_flag = 1;
			break;
		case MACHINE_RESET_FLAG:
			machine->std_flag = 0;
			break;
		case MACHINE_FLAG_SKIP:
			if (machine->std_flag)
				chunk_skip(chunk, 0);
			break;
		case MACHINE_NEW_FRAME:
			if (machine->call_size == MAX_CALLS)
				return machine->last_err = ERROR_STACK_OVERFLOW;
			if (!init_var_context(&machine->var_stack[machine->call_size++], &machine->garbage_collector))
				return ERROR_OUT_OF_MEMORY;
			break;
		case MACHINE_CLEAN:
			if (machine->call_size == 0)
				return machine->last_err = ERROR_INSUFFICIENT_CALLS;
			free_var_context(&machine->var_stack[--machine->call_size]);
			break;
		case MACHINE_BUILD_COL:
			if (!build_collection(machine, chunk))
				return machine->last_err;
			break;
		case MACHINE_BUILD_PROTO: {
			unsigned long id = chunk_read_ulong(chunk);
			unsigned long properties = chunk_read_ulong(chunk);
			struct record_prototype* prototype = malloc(sizeof(struct record_prototype));
			init_record_prototype(prototype, id);
			while (properties--)
				if (!record_append_property(prototype, chunk_read_ulong(chunk))) {
					machine->last_err = ERROR_PROPERTY_REDEFINE;
					return 0;
				}
			if (!cache_insert_prototype(&machine->global_cache, id, prototype))
				return machine->last_err = ERROR_RECORD_REDEFINE;
			break;
		}
		case MACHINE_BUILD_RECORD: {
			unsigned long id = chunk_read_ulong(chunk);
			struct record* new_rec = malloc(sizeof(struct record));
			if (new_rec == NULL)
				return machine->last_err = ERROR_OUT_OF_MEMORY;
			if (!cache_init_record(&machine->global_cache, id, new_rec))
				return machine->last_err = ERROR_RECORD_UNDEFINED;
			for (unsigned char i = 0; i < new_rec->prototype->size; i++) {
				struct value* property = malloc(sizeof(struct value));
				if(property == NULL)
					return machine->last_err = ERROR_OUT_OF_MEMORY;
				init_null_value(property);
				new_rec->properties[i] = property;
			}
			struct value* rec_val = malloc(sizeof(struct value));
			if(rec_val == NULL)
				return machine->last_err = ERROR_OUT_OF_MEMORY;
			struct object obj;
			init_object_rec(&obj, new_rec);
			init_obj_value(rec_val, obj);
			push_eval(machine, rec_val, 0);
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
			if (machine->eval_flags[machine->evals - 1]) {
				gc_register_trace(&machine->garbage_collector, machine->evaluation_stack[machine->evals - 1]);
			}
			break;
		case MACHINE_POP:
			machine->evals--;
			FREE_EVAL(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
			break;
		case MACHINE_CALL_EXTERN:
			if (!eval_builtin(machine, chunk))
				return machine->last_err;
			break;
		}
		chunk_read(chunk);
	}
	return 0;
}