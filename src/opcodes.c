#include <stdlib.h>
#include <string.h>
#include "include/error.h"
#include "include/hash.h"
#include "include/runtime/machine.h"
#include "include/runtime/operators.h"
#include "include/runtime/opcodes.h"

#define NULL_CHECK(PTR, ERROR) if((PTR) == NULL) { machine->last_err = ERROR; return 0; }
#define STACK_CHECK if(machine->evals == MACHINE_MAX_EVALS || machine->constants == MACHINE_MAX_EVALS || machine->positions == MACHINE_MAX_POSITIONS || machine->call_size == MACHINE_MAX_CALLS) { machine->last_err = ERROR_STACK_OVERFLOW; return 0; }
#define MATCH_EVALS(MIN_EVALS) if(machine->evals < MIN_EVALS) { machine->last_err = ERROR_INSUFFICIENT_EVALS; return 0; }

#define PUSH_EVAL(VALUE_PTR) if(!push_eval(machine, VALUE_PTR, 1)) { return 0; }

#define MACHINE_ERROR(ERROR) {machine->last_err = ERROR; return 0;}
#define DECL_OPCODE_HANDLER(METHOD_NAME) static const int METHOD_NAME(struct machine* machine, struct chunk* chunk)

DECL_OPCODE_HANDLER(opcode_load_const) {
	struct value constant = chunk_read_value(chunk);
	PUSH_EVAL(&constant);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_load_var) {
	struct value* var_ptr = retrieve_var(&machine->var_stack[machine->call_size - 1], chunk_read_ulong(chunk));

	NULL_CHECK(var_ptr, ERROR_VARIABLE_UNDEFINED);

	PUSH_EVAL(var_ptr);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_store_var) {
	MATCH_EVALS(1);
	uint64_t id = chunk_read_ulong(chunk);

	struct value* src = pop_eval(machine);
	NULL_CHECK(src, ERROR_INSUFFICIENT_EVALS);

	if (src->gc_flag == GARBAGE_UNINIT) {
		struct value* dest = retrieve_var(&machine->var_stack[machine->call_size - 1], id);
		if (dest) {
			free_value(dest);
			*dest = *src;
			NULL_CHECK(gc_register_children(&machine->garbage_collector, dest), ERROR_OUT_OF_MEMORY);
		}
		else if (!emplace_var(&machine->var_stack[machine->call_size - 1], id, gc_register_value(&machine->garbage_collector, *src)))
			MACHINE_ERROR(ERROR_OUT_OF_MEMORY);
	}
	else if (!emplace_var(&machine->var_stack[machine->call_size - 1], id, src))
		MACHINE_ERROR(ERROR_OUT_OF_MEMORY);

	return 1;
}

DECL_OPCODE_HANDLER(opcode_eval_bin_op) {
	MATCH_EVALS(2);

	struct value* value_b = pop_eval(machine);
	NULL_CHECK(value_b, ERROR_INSUFFICIENT_EVALS);
	struct value* value_a = pop_eval(machine);
	NULL_CHECK(value_a, ERROR_INSUFFICIENT_EVALS);

	enum binary_operator op = chunk_read(chunk);

	if (IS_RECORD(*value_a) || IS_RECORD(*value_b)) {
		struct value* record_val = IS_RECORD(*value_a) ? value_a : value_b;
		struct value* operand = IS_RECORD(*value_a) ? value_b : value_a;

		uint64_t pos = cache_retrieve_pos(&machine->global_cache, combine_hash(combine_hash(combine_hash((uint64_t)op, BINARY_OVERLOAD_CONST), 2), record_val->payload.object.ptr.record->prototype->identifier));
		NULL_CHECK(pos, ERROR_LABEL_UNDEFINED);

		STACK_CHECK;
		machine->position_stack[machine->positions] = chunk->pos;
		machine->position_flags[machine->positions++] = 1;

		PUSH_EVAL(operand);
		PUSH_EVAL(record_val);
		chunk_jump_to(chunk, pos);
	}
	else {
		struct value result = invoke_binary_op(op, value_a, value_b);
		PUSH_EVAL(&result);
	}
	return 1;
}

DECL_OPCODE_HANDLER(opcode_eval_uni_op) {
	MATCH_EVALS(1);

	struct value* operand = pop_eval(machine);
	NULL_CHECK(operand, ERROR_INSUFFICIENT_EVALS);
	enum unary_operator op = chunk_read(chunk);

	if (!(op == OPERATOR_COPY && operand->type == VALUE_TYPE_OBJ)) {
		if (IS_RECORD(*operand)) {
			uint64_t pos = cache_retrieve_pos(&machine->global_cache, combine_hash(combine_hash(combine_hash((uint64_t)op, UNARY_OVERLOAD_CONST), 1), operand->payload.object.ptr.record->prototype->identifier));
			NULL_CHECK(pos, ERROR_LABEL_UNDEFINED);

			STACK_CHECK;
			machine->position_stack[machine->positions] = chunk->pos;
			machine->position_flags[machine->positions++] = 1;

			PUSH_EVAL(operand);
			chunk_jump_to(chunk, pos);
		}
		else {
			struct value result = invoke_unary_op(op, operand, machine);
			push_eval(machine, &result, 0);
		}
	}
	else
		PUSH_EVAL(operand);

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

	if (!IS_RECORD(*record_eval))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct record* current_record = record_eval->payload.object.ptr.record;
	unsigned long id = chunk_read_ulong(chunk);

	STACK_CHECK;
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
	uint64_t id = chunk_read_ulong(chunk);
	if (!cache_insert_label(&machine->global_cache, id, chunk->pos))
		MACHINE_ERROR(ERROR_LABEL_REDEFINE);
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
	STACK_CHECK;
	NULL_CHECK(pop_eval(machine), ERROR_INSUFFICIENT_EVALS);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_get_index) {
	MATCH_EVALS(2);

	struct value* index_val = pop_eval(machine);
	NULL_CHECK(index_val, ERROR_INSUFFICIENT_EVALS);
	struct value* collection_val = pop_eval(machine);
	NULL_CHECK(collection_val, ERROR_INSUFFICIENT_EVALS);

	if (index_val->type != VALUE_TYPE_NUM || !IS_COLLECTION(*collection_val))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct collection* collection = collection_val->payload.object.ptr.collection;
	uint64_t index = index_val->payload.numerical;

	if (index >= collection->size)
		MACHINE_ERROR(ERROR_INDEX_OUT_OF_RANGE);

	PUSH_EVAL(collection->inner_collection[index]);

	return 1;
}

DECL_OPCODE_HANDLER(opcode_set_index) {
	MATCH_EVALS(3);

	struct value* set_val = pop_eval(machine);
	NULL_CHECK(set_val, ERROR_INSUFFICIENT_EVALS);
	struct value* index_val = pop_eval(machine);
	NULL_CHECK(index_val, ERROR_INSUFFICIENT_EVALS);
	struct value* collection_val = pop_eval(machine);
	NULL_CHECK(collection_val, ERROR_INSUFFICIENT_EVALS);

	if (index_val->type != VALUE_TYPE_NUM || collection_val->type != VALUE_TYPE_OBJ || collection_val->payload.object.type != OBJ_TYPE_COL)
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct collection* collection = collection_val->payload.object.ptr.collection;
	uint64_t index = index_val->payload.numerical;

	if (index >= collection->size)
		MACHINE_ERROR(ERROR_INDEX_OUT_OF_RANGE);

	if (set_val->gc_flag == GARBAGE_UNINIT) {
		if(collection->inner_collection[index] == GARBAGE_UNINIT)
			collection->inner_collection[index] = set_val;
		else {
			free_value(collection->inner_collection[index]);
			*collection->inner_collection[index] = *set_val;
			NULL_CHECK(gc_register_children(&machine->garbage_collector, collection->inner_collection[index]), ERROR_OUT_OF_MEMORY);
		}
	}
	else
		collection->inner_collection[index] = set_val;

	return 1;
}

DECL_OPCODE_HANDLER(opcode_get_property) {
	MATCH_EVALS(1);

	struct value* record_eval = pop_eval(machine);
	NULL_CHECK(record_eval, ERROR_INSUFFICIENT_EVALS);

	if (!IS_RECORD(*record_eval))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);

	struct value* property_val = record_get_property(record_eval->payload.object.ptr.record, chunk_read_ulong(chunk));

	NULL_CHECK(property_val, ERROR_PROPERTY_UNDEFINED);

	PUSH_EVAL(property_val);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_set_property) {
	MATCH_EVALS(2);

	uint64_t property = chunk_read_ulong(chunk);

	struct value* set_val = pop_eval(machine);
	NULL_CHECK(set_val, ERROR_INSUFFICIENT_EVALS);
	struct value* record_eval = pop_eval(machine);
	NULL_CHECK(record_eval, ERROR_INSUFFICIENT_EVALS);

	if (!IS_RECORD(*record_eval))
		MACHINE_ERROR(ERROR_UNEXPECTED_TYPE);
	struct record* record = record_eval->payload.object.ptr.record;
	
	if (set_val->gc_flag == GARBAGE_UNINIT) {
		struct value* property_val = record_get_property(record, property);
		
		if (property_val->gc_flag == GARBAGE_UNINIT) {
			if (!record_set_property(record, property, set_val))
				MACHINE_ERROR(ERROR_PROPERTY_UNDEFINED);
		}
		else {
			free_value(property_val);
			*property_val = *set_val;
			NULL_CHECK(gc_register_children(&machine->garbage_collector, property_val), ERROR_OUT_OF_MEMORY);
		}
	}
	else if (!record_set_property(record, property, set_val))
		MACHINE_ERROR(ERROR_PROPERTY_UNDEFINED);

	return 1;
}

DECL_OPCODE_HANDLER(opcode_eval_builtin) {
	uint64_t id = chunk_read_ulong(chunk);
	uint64_t arguments = chunk_read_ulong(chunk);

	MATCH_EVALS(arguments);

	struct value result = cache_invoke_builtin(&machine->global_cache, id, &machine->evaluation_stack[machine->evals - arguments], arguments, machine);

	while (arguments--)
		NULL_CHECK(pop_eval(machine), ERROR_INSUFFICIENT_EVALS);
	
	PUSH_EVAL(&result);
	return 1;
}

DECL_OPCODE_HANDLER(opcode_build_collection) {
	uint64_t req_size = chunk_read_ulong(chunk);
	MATCH_EVALS(req_size);

	struct collection* collection = malloc(sizeof(struct collection));
	NULL_CHECK(collection, ERROR_OUT_OF_MEMORY);
	NULL_CHECK(init_collection(collection, req_size), ERROR_OUT_OF_MEMORY);

	while (req_size--)
		NULL_CHECK(collection->inner_collection[req_size] = pop_eval(machine), ERROR_INSUFFICIENT_EVALS);

	struct value new_val;

	struct object obj;
	init_object_col(&obj, collection);
	init_obj_value(&new_val, obj);

	return push_eval(machine, &new_val, 1);
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
	
	if (!cache_init_record(&machine->global_cache, id, new_rec, machine))
		MACHINE_ERROR(ERROR_RECORD_UNDEFINED);
	
	struct value rec_val;
	struct object obj;

	init_object_rec(&obj, new_rec);
	init_obj_value(&rec_val, obj);
	
	push_eval(machine, &rec_val, 0);
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