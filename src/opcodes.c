#include <stdlib.h>
#include <string.h>
#include "include/error.h"
#include "include/hash.h"
#include "include/runtime/machine.h"
#include "include/runtime/operators.h"
#include "include/runtime/opcodes.h"

#define NULL_CHECK(PTR, ERROR) if(PTR == NULL) { machine->last_err = ERROR; return 0; }
#define STACK_CHECK if(machine->evals == MACHINE_MAX_EVALS || machine->positions == MACHINE_MAX_POSITIONS || machine->call_size == MACHINE_MAX_CALLS) { machine->last_err = ERROR_STACK_OVERFLOW; return 0; }

#define MATCH_EVALS(MIN_EVALS) if(machine->evals < MIN_EVALS) { machine->last_err = ERROR_INSUFFICIENT_EVALS; return 0; }
#define FREE_EVAL(EVAL, FLAGS) if(!FLAGS) { free_value(EVAL); free(EVAL); }

#define MACHINE_ERROR(ERROR) {machine->last_err = ERROR; return 0;}

#define DECL_OPCODE_HANDLER(METHOD_NAME) static const int METHOD_NAME(struct machine* machine, struct chunk* chunk)

inline static const int push_eval(struct machine* machine, struct value* value, char flags) {
	STACK_CHECK;
	machine->evaluation_stack[machine->evals] = value;
	machine->eval_flags[machine->evals++] = flags;
	if (!flags)
		value->gc_flag = GARBAGE_UNINIT;
	return 1;
}

inline static const int condition_check(struct machine* machine) {
	MATCH_EVALS(1);
	struct value* valptr = machine->evaluation_stack[--machine->evals];
	int cond = 1;
	if (valptr->type == VALUE_TYPE_NULL || (valptr->type == VALUE_TYPE_NUM && valptr->payload.numerical == 0))
		cond = 0;
	FREE_EVAL(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
	return cond;
}

DECL_OPCODE_HANDLER(opcode_load_const) {
	struct value* to_push = malloc(sizeof(struct value));
	NULL_CHECK(to_push, ERROR_OUT_OF_MEMORY);
	copy_value(to_push, chunk_read_size(chunk, sizeof(struct value)));
	if (!push_eval(machine, to_push, 0))
		MACHINE_ERROR(ERROR_STACK_OVERFLOW);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_load_var) {
	struct value* var_ptr = retrieve_var(&machine->var_stack[machine->call_size - 1], chunk_read_ulong(chunk));
	NULL_CHECK(var_ptr, ERROR_VARIABLE_UNDEFINED);
	if (!push_eval(machine, var_ptr, 1))
		MACHINE_ERROR(ERROR_STACK_OVERFLOW);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_store_var) {
	MATCH_EVALS(1);
	uint64_t id = chunk_read_ulong(chunk);
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

DECL_OPCODE_HANDLER(opcode_eval_bin_op) {
	MATCH_EVALS(2);

	struct value* value_b = machine->evaluation_stack[--machine->evals];
	char flag_b = machine->eval_flags[machine->evals];
	struct value* value_a = machine->evaluation_stack[--machine->evals];
	char flag_a = machine->eval_flags[machine->evals];
	enum binary_operator op = chunk_read(chunk);
	struct value* result = invoke_binary_op(op, value_a, value_b);
	FREE_EVAL(value_a, flag_a);
	FREE_EVAL(value_b, flag_b);
	NULL_CHECK(result, ERROR_UNEXPECTED_TYPE);
	if (!push_eval(machine, result, 0))
		return 0;
	return 1;
}

DECL_OPCODE_HANDLER(opcode_eval_uni_op) {
	MATCH_EVALS(1);
	enum unary_operator op = chunk_read(chunk);
	struct value* result = invoke_unary_op(op, machine->evaluation_stack[--machine->evals]);
	NULL_CHECK(result, ERROR_UNEXPECTED_TYPE);
	if (result != machine->evaluation_stack[machine->evals]) {
		FREE_EVAL(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
		if (!push_eval(machine, result, 0))
			return 0;
	}
	else
		push_eval(machine, result, machine->eval_flags[machine->evals]);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_mark) {
	STACK_CHECK;
	machine->position_stack[machine->positions] = chunk->pos - 1;
	machine->position_flags[machine->positions++] = 0;
	return 1;
}

DECL_OPCODE_HANDLER(opcode_goto) {
	STACK_CHECK;
	machine->position_stack[machine->positions] = chunk->pos + sizeof(uint64_t);
	machine->position_flags[machine->positions++] = 1;
	uint64_t pos = cache_retrieve_pos(&machine->global_cache, chunk_read_ulong(chunk));
	NULL_CHECK(pos, ERROR_LABEL_UNDEFINED);
	chunk_jump_to(chunk, pos);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_goto_as) {
	MATCH_EVALS(1);

	struct value* record_eval = machine->evaluation_stack[machine->evals - 1];
	char record_flag = machine->eval_flags[machine->evals - 1];

	if (!IS_RECORD(record_eval))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	STACK_CHECK;
	machine->position_stack[machine->positions] = chunk->pos + sizeof(uint64_t);
	machine->position_flags[machine->positions++] = 1;

	struct record* current_record = record_eval->payload.object.ptr.record;
	unsigned long id = chunk_read_ulong(chunk);
	while (record_eval != NULL)
	{
		uint64_t pos = cache_retrieve_pos(&machine->global_cache, combine_hash(id, current_record->prototype->identifier));
		if (pos) {
			chunk_jump_to(chunk, pos);
			return 1;
		}
		record_eval = record_get_property(current_record, RECORD_BASE_PROPERTY);
		if (record_eval)
			if(!IS_RECORD(record_eval))
				MACHINE_ERROR(ERROR_UNEXPECTED_TYPE)
			else
				current_record = record_eval->payload.object.ptr.record;
	}
	MACHINE_ERROR(ERROR_LABEL_UNDEFINED);
}

DECL_OPCODE_HANDLER(opcode_return_goto) {
	while (!machine->position_flags[machine->positions - 1])
		machine->positions--;
	chunk_jump_to(chunk, machine->position_stack[--machine->positions]);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_label) {
	uint64_t id = chunk_read_ulong(chunk);
	if (!cache_insert_label(&machine->global_cache, id, chunk->pos))
		MACHINE_ERROR(ERROR_LABEL_UNDEFINED);
	chunk_read(chunk);
	chunk_skip(chunk, 1);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_cond_skip) {
	if (!condition_check(machine))
		chunk_skip(chunk, 0);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_cond_return) {
	--machine->positions;
	if (condition_check(machine))
		chunk_jump_to(chunk, machine->position_stack[machine->positions]);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_flag) {
	machine->std_flag = 1;
	return 1;
}

DECL_OPCODE_HANDLER(opcode_reset_flag) {
	machine->std_flag = 0;
	return 1;
}

DECL_OPCODE_HANDLER(opcode_flag_skip) {
	if (machine->std_flag)
		chunk_skip(chunk, 0);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_new_frame) {
	STACK_CHECK;
	if (!init_var_context(&machine->var_stack[machine->call_size++], &machine->garbage_collector))
		MACHINE_ERROR(ERROR_OUT_OF_MEMORY);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_clean) {
	if (machine->call_size == 0)
		MACHINE_ERROR(ERROR_INSUFFICIENT_CALLS);
	free_var_context(&machine->var_stack[--machine->call_size]);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_trace) {
	if (machine->eval_flags[machine->evals - 1])
		gc_register_trace(&machine->garbage_collector, machine->evaluation_stack[machine->evals - 1]);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_pop) {
	if (!machine->evals)
		MACHINE_ERROR(ERROR_INSUFFICIENT_EVALS);
	machine->evals--;
	FREE_EVAL(machine->evaluation_stack[machine->evals], machine->eval_flags[machine->evals]);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_get_index) {
	MATCH_EVALS(2);

	struct value* index_val = machine->evaluation_stack[--machine->evals];
	char index_flag = machine->eval_flags[machine->evals];
	struct value* collection_val = machine->evaluation_stack[--machine->evals];
	char collection_flag = machine->eval_flags[machine->evals];

	if (index_val->type != VALUE_TYPE_NUM || !IS_COLLECTION(collection_val))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct collection* collection = collection_val->payload.object.ptr.collection;
	uint64_t index = index_val->payload.numerical;

	if (index >= collection->size)
		MACHINE_ERROR(ERROR_INDEX_OUT_OF_RANGE);

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

DECL_OPCODE_HANDLER(opcode_set_index) {
	MATCH_EVALS(3);

	struct value* set_val = machine->evaluation_stack[--machine->evals];
	char set_flag = machine->eval_flags[machine->evals];
	struct value* index_val = machine->evaluation_stack[--machine->evals];
	char index_flag = machine->eval_flags[machine->evals];
	struct value* collection_val = machine->evaluation_stack[--machine->evals];
	char collection_flag = machine->eval_flags[machine->evals];

	if (index_val->type != VALUE_TYPE_NUM || collection_val->type != VALUE_TYPE_OBJ || collection_val->payload.object.type != OBJ_TYPE_COL)
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct collection* collection = collection_val->payload.object.ptr.collection;
	uint64_t index = index_val->payload.numerical;

	if (index >= collection->size)
		MACHINE_ERROR(ERROR_INDEX_OUT_OF_RANGE);

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

DECL_OPCODE_HANDLER(opcode_set_property) {
	MATCH_EVALS(2);

	uint64_t property = chunk_read_ulong(chunk);

	struct value* set_val = machine->evaluation_stack[--machine->evals];
	char set_flag = machine->eval_flags[machine->evals];
	struct value* record_eval = machine->evaluation_stack[--machine->evals];
	char record_flag = machine->eval_flags[machine->evals];

	if (!IS_RECORD(record_eval))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct record* record = record_eval->payload.object.ptr.record;
	struct value* property_val = record_get_property(record, property);
	if (!property_val)
		MACHINE_ERROR(ERROR_PROPERTY_UNDEFINED);

	if (set_flag) {
		if (property_val->gc_flag == GARBAGE_UNINIT) {
			free_value(property_val);
			free(property_val);
		}
		if (!record_set_property(record, property, set_val))
			MACHINE_ERROR(ERROR_PROPERTY_UNDEFINED);
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

DECL_OPCODE_HANDLER(opcode_get_property) {
	MATCH_EVALS(1);

	struct value* record_eval = machine->evaluation_stack[--machine->evals];
	char record_flag = machine->eval_flags[machine->evals];

	if (!IS_RECORD(record_eval))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct value* toreturn = NULL;
	struct value* property_val = record_get_property(record_eval->payload.object.ptr.record, chunk_read_ulong(chunk));
	if (!property_val)
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

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

DECL_OPCODE_HANDLER(opcode_eval_builtin) {
	uint64_t id = chunk_read_ulong(chunk);
	uint64_t arguments = chunk_read_ulong(chunk);

	MATCH_EVALS(arguments);

	struct value* result = cache_invoke_builtin(&machine->global_cache, id, &machine->evaluation_stack[machine->evals - arguments], arguments);
	NULL_CHECK(result, ERROR_LABEL_UNDEFINED);
	for (uint_fast64_t i = machine->evals - arguments; i < machine->evals; i++)
		FREE_EVAL(machine->evaluation_stack[i], machine->eval_flags[i]);
	machine->evals -= arguments;
	return push_eval(machine, result, 0);
}

DECL_OPCODE_HANDLER(opcode_build_collection) {
	uint64_t req_size = chunk_read_ulong(chunk);
	MATCH_EVALS(req_size);

	struct collection* collection = malloc(sizeof(struct collection));
	NULL_CHECK(collection, ERROR_OUT_OF_MEMORY);
	NULL_CHECK(init_collection(collection, req_size), ERROR_OUT_OF_MEMORY);

	while (req_size--)
		collection->inner_collection[req_size] = machine->evaluation_stack[--machine->evals];

	struct value* new_val = malloc(sizeof(struct value));
	NULL_CHECK(new_val, ERROR_OUT_OF_MEMORY);

	struct object obj;
	init_object_col(&obj, collection);
	init_obj_value(new_val, obj);

	return push_eval(machine, new_val, 0);
}

DECL_OPCODE_HANDLER(opcode_build_record_proto) {
	uint64_t id = chunk_read_ulong(chunk);
	uint64_t properties = chunk_read_ulong(chunk);
	struct record_prototype* prototype = malloc(sizeof(struct record_prototype));
	NULL_CHECK(prototype, ERROR_OUT_OF_MEMORY);
	init_record_prototype(prototype, id);
	while (properties--)
		if (!record_append_property(prototype, chunk_read_ulong(chunk)))
			MACHINE_ERROR(ERROR_PROPERTY_REDEFINE);
	if (!cache_insert_prototype(&machine->global_cache, id, prototype))
		MACHINE_ERROR(ERROR_RECORD_REDEFINE);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_build_record) {
	uint64_t id = chunk_read_ulong(chunk);
	struct record* new_rec = malloc(sizeof(struct record));
	NULL_CHECK(new_rec, ERROR_OUT_OF_MEMORY);
	if (!cache_init_record(&machine->global_cache, id, new_rec))
		MACHINE_ERROR(ERROR_RECORD_UNDEFINED);
	struct value* rec_val = malloc(sizeof(struct value));
	NULL_CHECK(rec_val, ERROR_OUT_OF_MEMORY);
	struct object obj;
	init_object_rec(&obj, new_rec);
	init_obj_value(rec_val, obj);
	push_eval(machine, rec_val, 0);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_inherit_record) {
	uint64_t child_id = chunk_read_ulong(chunk);
	uint64_t parent_id = chunk_read_ulong(chunk);
	
	if (!cache_merge_proto(&machine->global_cache, child_id, parent_id))
		MACHINE_ERROR(ERROR_RECORD_UNDEFINED);
	return 1;
}

DECL_OPCODE_HANDLER((*opcode_handler[30])) = {
	opcode_load_const,
	opcode_load_var,

	opcode_store_var,

	opcode_eval_bin_op,
	opcode_eval_uni_op,

	NULL,

	opcode_mark,

	opcode_goto,
	opcode_goto_as,
	opcode_return_goto,
	opcode_label,

	opcode_cond_skip,
	opcode_cond_return,

	opcode_flag,
	opcode_reset_flag,
	opcode_flag_skip,

	opcode_new_frame,
	opcode_clean,
	opcode_trace,
	opcode_pop,

	opcode_build_collection,
	opcode_build_record_proto,
	opcode_build_record,
	opcode_inherit_record,

	opcode_get_index,
	opcode_set_index,
	opcode_get_property,
	opcode_set_property,

	opcode_eval_builtin,
	NULL
};

const int handle_opcode(enum op_code op, struct machine* machine, struct chunk* chunk) {
	if (op >= MACHINE_END || op < 0)
		MACHINE_ERROR(ERROR_UNRECOGNIZED_OPCODE);
	if(opcode_handler[op])
		return (*opcode_handler[op])(machine, chunk);
	return 1;
}