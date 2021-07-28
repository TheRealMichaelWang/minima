#include <stdlib.h>
#include <string.h>
#include "include/error.h"
#include "include/compiler/chunk.h"

void init_chunk(struct chunk* chunk, uint8_t* code, const uint64_t size) {
	chunk->pos = 0;
	chunk->size = size;
	chunk->code = code;
	chunk_read_opcode(chunk);
}

void free_chunk(struct chunk* chunk) {
	free(chunk->code);
}

const int init_chunk_builder(struct chunk_builder* builder) {
	ERROR_ALLOC_CHECK(builder->code_buf = malloc(100 * sizeof(uint8_t)));
	builder->size = 0;
	builder->alloc_size = 100;
}

struct chunk build_chunk(struct chunk_builder* builder) {
	struct chunk mychunk;
	init_chunk(&mychunk, builder->code_buf, builder->size);
	return mychunk;
}

const int chunk_write_ulong(struct chunk_builder* chunk_builder, const uint64_t ulong) {
	return chunk_write_size(chunk_builder, &ulong, sizeof(uint64_t));
}

const int chunk_write_value(struct chunk_builder* chunk_builder, struct value value) {
	if (value.type == VALUE_TYPE_OBJ)
		return 0;
	chunk_write_size(chunk_builder, &value, sizeof(struct value));
	return 1;
}

const int chunk_write_opcode(struct chunk_builder* chunk_builder, enum op_code opcode) {
	uint8_t small_cast = (uint8_t)opcode;
	ERROR_ALLOC_CHECK(chunk_write_size(chunk_builder, &small_cast, sizeof(uint8_t)));
	if (opcode == MACHINE_LABEL || opcode == MACHINE_COND_SKIP || opcode == MACHINE_FLAG_SKIP)
		ERROR_ALLOC_CHECK(chunk_write_size(chunk_builder, NULL, sizeof(uint64_t)));
	return 1;
}

const int chunk_write_bin_op(struct chunk_builder* chunk_builder, enum binary_operator bin_op) {
	uint8_t small_cast = (uint8_t)bin_op;
	return chunk_write_size(chunk_builder, &small_cast, sizeof(uint8_t));
}

const int chunk_write_uni_op(struct chunk_builder* chunk_builder, enum unary_operator uni_op) {
	uint8_t small_cast = (uint8_t)uni_op;
	return chunk_write_size(chunk_builder, &small_cast, sizeof(uint8_t));
}

const int chunk_write_chunk(struct chunk_builder* dest, struct chunk src, const int free_chunk) {
	ERROR_ALLOC_CHECK(chunk_write_size(dest, src.code, src.size));
	if (free_chunk)
		free(src.code);
	return 1;
}

const void* chunk_write_size(struct chunk_builder* chunk_builder, const void* ptr, const uint64_t size) {
	if (chunk_builder->size + size >= chunk_builder->alloc_size) {
		void* new_ptr = realloc(chunk_builder->code_buf, (chunk_builder->alloc_size * 2 + size) * sizeof(uint8_t));
		ERROR_ALLOC_CHECK(new_ptr);
		chunk_builder->code_buf = new_ptr;
		chunk_builder->alloc_size *= 2;
		chunk_builder->alloc_size += size;
	}
	void* dest = &chunk_builder->code_buf[chunk_builder->size];
	if(ptr)
		memcpy(dest, ptr, size);
	chunk_builder->size += size;
	return dest;
}

const uint64_t chunk_read_ulong(struct chunk* chunk) {
	return *(uint64_t*)chunk_read_size(chunk, sizeof(uint64_t));
}

const struct value chunk_read_value(struct chunk* chunk) {
	return *(struct value*)chunk_read_size(chunk, sizeof(struct value));
}

const enum op_code chunk_read_opcode(struct chunk* chunk) {
	if (chunk->pos >= chunk->size)
		return chunk->last_code = MACHINE_END;
	return chunk->last_code = *(uint8_t*)chunk_read_size(chunk, sizeof(uint8_t));
}

const enum binary_operator chunk_read_bin_op(struct chunk* chunk) {
	return  *(uint8_t*)chunk_read_size(chunk, sizeof(uint8_t));
}

const enum binary_operator chunk_read_uni_op(struct chunk* chunk) {
	return *(uint8_t*)chunk_read_size(chunk, sizeof(uint8_t));
}

const void* chunk_read_size(struct chunk* chunk, const uint64_t size) {
	if (chunk->pos + size >= chunk->size) {
		chunk->last_code = MACHINE_END;
		if(chunk->pos + size > chunk->size)
			return NULL;
	}
	const void* position = &chunk->code[chunk->pos];
	chunk->pos += size;
	return position;
}

void chunk_jump_to(struct chunk* chunk, const uint64_t pos) {
	if (pos >= chunk->size) {
		chunk->pos = chunk->size;
		chunk->last_code = MACHINE_END;
	}
	chunk->pos = pos;
}

const int chunk_optimize(struct chunk* chunk, uint64_t offset) {
	uint64_t** skip_stack = malloc(64);
	uint8_t skips = 0;

	while (chunk->last_code != MACHINE_END)
	{
		enum op_code op = chunk->last_code;
		switch (op)
		{
		case MACHINE_LABEL:
		case MACHINE_COND_SKIP:
		case MACHINE_FLAG_SKIP: {
			skip_stack[skips++] = chunk_read_size(chunk, sizeof(uint64_t));
			if(op == MACHINE_LABEL)
				chunk_read_size(chunk, sizeof(uint64_t));
			break;
		}
		case MACHINE_END_SKIP: {
			*skip_stack[--skips] = chunk->pos - 1 + offset;
			break;
		}
		case MACHINE_CALL_EXTERN:
		case MACHINE_INHERIT_REC:
			chunk_read_size(chunk, sizeof(uint64_t));
		case MACHINE_GOTO:
		case MACHINE_GOTO_AS:
		case MACHINE_STORE_VAR:
		case MACHINE_LOAD_VAR:
		case MACHINE_BUILD_COL:
		case MACHINE_SET_PROPERTY:
		case MACHINE_GET_PROPERTY:
		case MACHINE_BUILD_RECORD:
			chunk_read_size(chunk, sizeof(uint64_t));
			break;
		case MACHINE_LOAD_CONST:
			chunk_read_size(chunk, sizeof(struct value));
			break;
		case MACHINE_EVAL_BIN_OP:
		case MACHINE_EVAL_UNI_OP:
			chunk_read_size(chunk, sizeof(uint8_t));
			break;
		case MACHINE_BUILD_PROTO: {
			chunk_read_size(chunk, sizeof(uint64_t));
			uint64_t i = chunk_read_ulong(chunk);
			while (i--)
				chunk_read_size(chunk, sizeof(uint64_t));
			break;
		}
		}
		chunk_read_opcode(chunk);
	}
	return skips == 0;
}