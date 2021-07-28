#include <stdlib.h>
#include <string.h>
#include "include/error.h"
#include "include/hash.h"
#include "include/runtime/machine.h"
#include "include/runtime/operators.h"
#include "include/runtime/opcodes.h"

#define NULL_CHECK(PTR, ERROR) if(!(PTR)) { machine->last_err = ERROR; return 0; }

#define MPUSH_EVAL(EVAL) NULL_CHECK(machine_push_eval(machine, EVAL), ERROR_STACK_OVERFLOW)
#define MPUSH_CONST(EVAL, HAS_CHILDREN) NULL_CHECK(machine_push_const(machine, EVAL, HAS_CHILDREN), ERROR_STACK_OVERFLOW);

#define MACHINE_ERROR(ERROR) {machine->last_err = ERROR; return 0;}
#define DECL_OPCODE_HANDLER(METHOD_NAME) static const int METHOD_NAME(struct machine* machine, struct chunk* chunk)

DECL_OPCODE_HANDLER(opcode_load_const) {
	MPUSH_CONST(chunk_read_value(chunk), 0);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_load_var) {
	struct value*var_ptr = retrieve_var(&machine->var_stack[machine->call_size - 1], chunk_read_ulong(chunk));
	NULL_CHECK(var_ptr, ERROR_VARIABLE_UNDEFINED);
	MPUSH_EVAL(var_ptr);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_store_var) {
	uint64_t id = chunk_read_ulong(chunk);

	struct value*src = machine_pop_eval(machine);
	NULL_CHECK(src, ERROR_INSUFFICIENT_EVALS);

	if (src->gc_flag == GARBAGE_CONSTANT) {
		struct value* dest = retrieve_var(&machine->var_stack[machine->call_size - 1], id);
		if (dest) {
			if (src->type == VALUE_TYPE_NULL) {
				if (!emplace_var(&machine->var_stack[machine->call_size - 1], id, gc_register_value(&machine->garbage_collector, *src)))
					MACHINE_ERROR(ERROR_OUT_OF_MEMORY);
			}
			else {
				free_value(dest);
				*dest = *src;
				NULL_CHECK(gc_register_children(&machine->garbage_collector, dest), ERROR_OUT_OF_MEMORY);
			}
		}
		else if (!emplace_var(&machine->var_stack[machine->call_size - 1], id, gc_register_value(&machine->garbage_collector, *src)))
			MACHINE_ERROR(ERROR_OUT_OF_MEMORY);
	}
	else if (!emplace_var(&machine->var_stack[machine->call_size - 1], id, src))
		MACHINE_ERROR(ERROR_OUT_OF_MEMORY);

	return 1;
}

DECL_OPCODE_HANDLER(opcode_eval_bin_op) {
	struct value*value_b = machine_pop_eval(machine);
	NULL_CHECK(value_b, ERROR_INSUFFICIENT_EVALS);
	struct value*value_a = machine_pop_eval(machine);
	NULL_CHECK(value_a, ERROR_INSUFFICIENT_EVALS);

	enum binary_operator op = chunk_read_bin_op(chunk);

	if (IS_RECORD(*value_a) || IS_RECORD(*value_b)) {
		int in_order = IS_RECORD(*value_a);
		struct value* record_val = in_order ? value_a : value_b;
		struct value* operand = in_order ? value_b : value_a;

		struct value* original_record_operand = record_val;

		while (record_val != NULL)
		{
			uint64_t pos = cache_retrieve_pos(&machine->global_cache, combine_hash(combine_hash(combine_hash((uint64_t)op, BINARY_OVERLOAD), 3), record_val->payload.object.ptr.record->prototype->identifier));
			if (!pos)
				record_val = record_get_property(record_val->payload.object.ptr.record, RECORD_BASE_PROPERTY);
			else {
				if (machine->positions == MACHINE_MAX_POSITIONS)
					MACHINE_ERROR(ERROR_STACK_OVERFLOW);
				machine->position_stack[machine->positions] = chunk->pos;
				machine->position_flags[machine->positions++] = 1;

				MPUSH_EVAL(operand);
				MPUSH_EVAL(original_record_operand);
				MPUSH_CONST(NUM_VALUE(in_order), 0);

				chunk_jump_to(chunk, pos);
				return 1;
			}
		}
	}
	NULL_CHECK(invoke_binary_op(op, value_a, value_b, machine), ERROR_UNEXPECTED_TYPE);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_eval_uni_op) {
	struct value*operand = machine_pop_eval(machine);
	NULL_CHECK(operand, ERROR_INSUFFICIENT_EVALS);
	enum unary_operator op = chunk_read_uni_op(chunk);

	if (IS_RECORD(*operand)) {
		struct value* record_val = operand;
		struct value* original_record_operand = record_val;
		while (record_val != NULL)
		{
			uint64_t pos = cache_retrieve_pos(&machine->global_cache, combine_hash(combine_hash(combine_hash((uint64_t)op, UNARY_OVERLOAD), 1), record_val->payload.object.ptr.record->prototype->identifier));
			if (!pos)
				record_val = record_get_property(record_val->payload.object.ptr.record, RECORD_BASE_PROPERTY);
			else {
				if (machine->positions == MACHINE_MAX_POSITIONS)
					MACHINE_ERROR(ERROR_STACK_OVERFLOW);
				machine->position_stack[machine->positions] = chunk->pos;
				machine->position_flags[machine->positions++] = 1;

				MPUSH_EVAL(original_record_operand);
				chunk_jump_to(chunk, pos);
				return 1;
			}
		}
	}
	NULL_CHECK(invoke_unary_op(op, operand, machine), ERROR_UNEXPECTED_TYPE);
	return 1;
}

static struct value* argv[1000];

DECL_OPCODE_HANDLER(opcode_eval_builtin) {
	uint64_t id = chunk_read_ulong(chunk);
	uint64_t arguments = chunk_read_ulong(chunk);

	uint_fast64_t i = arguments;
	while (i--)
		NULL_CHECK(argv[i] = machine_pop_eval(machine), ERROR_INSUFFICIENT_EVALS);

	MPUSH_CONST(cache_invoke_builtin(&machine->global_cache, id, &argv, arguments, machine), 0);

	i = arguments;
	while (i--)
		if(argv[i]->gc_flag == GARBAGE_CONSTANT)
			free_value(argv[i]);

	return 1;
}

DECL_OPCODE_HANDLER(opcode_mark) {
	if (machine->positions == MACHINE_MAX_POSITIONS)
		MACHINE_ERROR(ERROR_STACK_OVERFLOW);
	machine->position_stack[machine->positions] = chunk->pos - 1;
	machine->position_flags[machine->positions++] = 0;
	return 1;
}

DECL_OPCODE_HANDLER(opcode_goto) {
	if (machine->positions == MACHINE_MAX_POSITIONS)
		MACHINE_ERROR(ERROR_STACK_OVERFLOW);
	machine->position_stack[machine->positions] = chunk->pos + sizeof(uint64_t);
	machine->position_flags[machine->positions++] = 1;
	uint64_t pos = cache_retrieve_pos(&machine->global_cache, chunk_read_ulong(chunk));
	NULL_CHECK(pos, ERROR_LABEL_UNDEFINED);
	chunk_jump_to(chunk, pos);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_goto_as) {
	NULL_CHECK(machine->evals, ERROR_INSUFFICIENT_EVALS);
	struct value* record_eval = machine->evaluation_stack[machine->evals - 1];

	if (!IS_RECORD(*record_eval))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct record* current_record = record_eval->payload.object.ptr.record;
	uint64_t id = chunk_read_ulong(chunk);

	if (machine->positions == MACHINE_MAX_POSITIONS)
		MACHINE_ERROR(ERROR_STACK_OVERFLOW);
	machine->position_stack[machine->positions] = chunk->pos; // +sizeof(uint64_t);
	machine->position_flags[machine->positions++] = 1;

	while (record_eval != NULL)
	{
		uint64_t pos = cache_retrieve_pos(&machine->global_cache, combine_hash(id, current_record->prototype->identifier));
		if (pos) {
			chunk_jump_to(chunk, pos);
			return 1;
		}
		record_eval = record_get_property(current_record, RECORD_BASE_PROPERTY);
		if (record_eval)
			if(!IS_RECORD(*record_eval))
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
	uint64_t pos = chunk_read_ulong(chunk);
	uint64_t id = chunk_read_ulong(chunk);
	if (!cache_insert_label(&machine->global_cache, id, chunk->pos))
		MACHINE_ERROR(ERROR_LABEL_REDEFINE);
	chunk_jump_to(chunk, pos);
	chunk_read_opcode(chunk);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_cond_skip) {
	uint64_t skip_pos = chunk_read_ulong(chunk);
	if (!machine_condition_check(machine)) {
		chunk_jump_to(chunk, skip_pos);
		chunk_read_opcode(chunk);
	}
	return 1;
}

DECL_OPCODE_HANDLER(opcode_cond_return) {
	--machine->positions;
	if (machine_condition_check(machine))
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
	uint64_t skip_pos = chunk_read_ulong(chunk);
	if (machine->std_flag) {
		chunk_jump_to(chunk, skip_pos);
		chunk_read_opcode(chunk);
	}
	return 1;
}

DECL_OPCODE_HANDLER(opcode_new_frame) {
	if (machine->call_size == MACHINE_MAX_CALLS)
		MACHINE_ERROR(ERROR_STACK_OVERFLOW);
	if (!init_var_context(&machine->var_stack[machine->call_size++], &machine->garbage_collector))
		MACHINE_ERROR(ERROR_OUT_OF_MEMORY);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_clean) {
	if (!machine->call_size)
		MACHINE_ERROR(ERROR_INSUFFICIENT_CALLS);
	free_var_context(&machine->var_stack[--machine->call_size]);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_trace) {
	NULL_CHECK(machine->evals, ERROR_INSUFFICIENT_EVALS);
	gc_register_trace(&machine->garbage_collector, machine->evaluation_stack[machine->evals - 1]);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_pop) {
	NULL_CHECK(machine->evals, ERROR_INSUFFICIENT_EVALS);
	NULL_CHECK(machine_pop_eval(machine), ERROR_INSUFFICIENT_EVALS);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_get_index) {
	struct value*index_val = machine_pop_eval(machine);
	NULL_CHECK(index_val, ERROR_INSUFFICIENT_EVALS);
	struct value*collection_val = machine_pop_eval(machine);
	NULL_CHECK(collection_val, ERROR_INSUFFICIENT_EVALS);

	if (index_val->type != VALUE_TYPE_NUM || !IS_COLLECTION(*collection_val))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct collection* collection = collection_val->payload.object.ptr.collection;
	uint64_t index = index_val->payload.numerical;

	if (index >= collection->size)
		MACHINE_ERROR(ERROR_INDEX_OUT_OF_RANGE);

	MPUSH_EVAL(collection->inner_collection[index]);

	return 1;
}

DECL_OPCODE_HANDLER(opcode_set_index) {
	struct value*set_val = machine_pop_eval(machine);
	NULL_CHECK(set_val, ERROR_INSUFFICIENT_EVALS);
	struct value*index_val = machine_pop_eval(machine);
	NULL_CHECK(index_val, ERROR_INSUFFICIENT_EVALS);
	struct value*collection_val = machine_pop_eval(machine);
	NULL_CHECK(collection_val, ERROR_INSUFFICIENT_EVALS);

	if (index_val->type != VALUE_TYPE_NUM || collection_val->type != VALUE_TYPE_OBJ || collection_val->payload.object.type != OBJ_TYPE_COL)
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct collection* collection = collection_val->payload.object.ptr.collection;
	uint64_t index = index_val->payload.numerical;

	if (index >= collection->size)
		MACHINE_ERROR(ERROR_INDEX_OUT_OF_RANGE);

	if (set_val->gc_flag == GARBAGE_CONSTANT) {
		if(collection->inner_collection[index] == GARBAGE_CONSTANT)
			collection->inner_collection[index] = set_val;
		else {
			if (set_val->type == VALUE_TYPE_NULL) {
				NULL_CHECK(collection->inner_collection[index] = gc_register_value(&machine->garbage_collector, *set_val), ERROR_OUT_OF_MEMORY);
			}
			else {
				free_value(collection->inner_collection[index]);
				*collection->inner_collection[index] = *set_val;
				NULL_CHECK(gc_register_children(&machine->garbage_collector, collection->inner_collection[index]), ERROR_OUT_OF_MEMORY);
			}
		}
	}
	else
		collection->inner_collection[index] = set_val;

	return 1;
}

DECL_OPCODE_HANDLER(opcode_get_property) {
	struct value*record_eval = machine_pop_eval(machine);
	NULL_CHECK(record_eval, ERROR_INSUFFICIENT_EVALS);

	if (!IS_RECORD(*record_eval))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct value* property_val = record_get_property(record_eval->payload.object.ptr.record, chunk_read_ulong(chunk));
	NULL_CHECK(property_val, ERROR_PROPERTY_UNDEFINED);
	MPUSH_EVAL(property_val);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_set_property) {
	uint64_t property = chunk_read_ulong(chunk);

	struct value*set_val = machine_pop_eval(machine);
	NULL_CHECK(set_val, ERROR_INSUFFICIENT_EVALS);
	struct value*record_eval = machine_pop_eval(machine);
	NULL_CHECK(record_eval, ERROR_INSUFFICIENT_EVALS);

	if (!IS_RECORD(*record_eval))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);
	struct record* record = record_eval->payload.object.ptr.record;
	
	if (set_val->gc_flag == GARBAGE_CONSTANT) {
		struct value* property_val = record_get_property(record, property);
		
		if (property_val->gc_flag == GARBAGE_CONSTANT) {
			if (!record_set_property(record, property, set_val))
				MACHINE_ERROR(ERROR_PROPERTY_UNDEFINED);
		}
		else {
			if (set_val->gc_flag == VALUE_TYPE_NULL) {
				if (!record_set_property(record, property, gc_register_value(&machine->garbage_collector, *set_val)))
					MACHINE_ERROR(ERROR_OUT_OF_MEMORY);
			}
			else {
				free_value(property_val);
				*property_val = *set_val;
				NULL_CHECK(gc_register_children(&machine->garbage_collector, property_val), ERROR_OUT_OF_MEMORY);
			}
		}
	}
	else if (!record_set_property(record, property, set_val))
		MACHINE_ERROR(ERROR_PROPERTY_UNDEFINED);

	return 1;
}

DECL_OPCODE_HANDLER(opcode_build_collection) {
	uint64_t req_size = chunk_read_ulong(chunk);

	struct collection* collection = malloc(sizeof(struct collection));
	NULL_CHECK(collection, ERROR_OUT_OF_MEMORY);
	NULL_CHECK(init_collection(collection, req_size), ERROR_OUT_OF_MEMORY);
	
	while (req_size--)
		NULL_CHECK(collection->inner_collection[req_size] = machine_pop_eval(machine), ERROR_INSUFFICIENT_EVALS);

	struct value new_val;

	struct object obj;
	init_object_col(&obj, collection);
	init_obj_value(&new_val, obj);
	
	MPUSH_CONST(new_val, 1);
	
	return 1;
}

DECL_OPCODE_HANDLER(opcode_build_record_proto) {
	uint64_t id = chunk_read_ulong(chunk);
	uint64_t properties = chunk_read_ulong(chunk);

	struct record_prototype* prototype = cache_insert_prototype(&machine->global_cache, id);
	NULL_CHECK(prototype, ERROR_OUT_OF_MEMORY);
	
	while (properties--)
		if (!record_append_property(prototype, chunk_read_ulong(chunk)))
			MACHINE_ERROR(ERROR_PROPERTY_REDEFINE);

	return 1;
}

DECL_OPCODE_HANDLER(opcode_build_record) {
	uint64_t id = chunk_read_ulong(chunk);

	struct record* new_rec = malloc(sizeof(struct record));
	NULL_CHECK(new_rec, ERROR_OUT_OF_MEMORY);
	
	if (!cache_init_record(&machine->global_cache, id, new_rec, machine))
		MACHINE_ERROR(ERROR_RECORD_UNDEFINED);
	
	struct value rec_val;
	struct object obj;

	init_object_rec(&obj, new_rec);
	init_obj_value(&rec_val, obj);
	
	MPUSH_CONST(rec_val, 0);
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