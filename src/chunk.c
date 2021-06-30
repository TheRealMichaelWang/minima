#include <stdlib.h>
#include <string.h>
#include "include/error.h"
#include "include/runtime/opcodes.h"
#include "include/runtime/value.h"
#include "include/compiler/chunk.h"

void init_chunk(struct chunk* chunk, char* code, const uint64_t size) {
	chunk->pos = 0;
	chunk->size = size;
	chunk->code = code;
	chunk_read(chunk);
}

void free_chunk(struct chunk* chunk) {
	chunk->pos = 0;
	chunk_read(chunk);
	while (chunk->last_code != MACHINE_END)
	{
		char op_code = chunk->last_code;
		if (op_code == MACHINE_LOAD_CONST) {
 			free_value(chunk_read_value(chunk));
			chunk_read(chunk);
		}
		else
			chunk_skip_ins(chunk);
	}
	free(chunk->code);
}

void init_chunk_builder(struct chunk_builder* builder) {
	builder->code_buf = malloc(100 * sizeof(char));
	builder->size = 0;
	builder->alloc_size = 100;
}

struct chunk build_chunk(struct chunk_builder* builder) {
	struct chunk mychunk;
	init_chunk(&mychunk, builder->code_buf, builder->size);
	return mychunk;
}

const int chunk_write(struct chunk_builder* chunk_builder, const char code) {
	if (chunk_builder->size == chunk_builder->alloc_size) {
		void* new_ptr = realloc(chunk_builder->code_buf, chunk_builder->alloc_size * 2);
		ERROR_ALLOC_CHECK(new_ptr);
		chunk_builder->code_buf = new_ptr;
		chunk_builder->alloc_size *= 2;
	}
	chunk_builder->code_buf[chunk_builder->size++] = code;
	return 1;
}


const int chunk_write_size(struct chunk_builder* chunk_builder, const void* ptr, const uint64_t size) {
	if (chunk_builder->size + size >= chunk_builder->alloc_size) {
		void* new_ptr = realloc(chunk_builder->code_buf, chunk_builder->alloc_size * 2 + size);
		ERROR_ALLOC_CHECK(new_ptr);
		chunk_builder->code_buf = new_ptr;
		chunk_builder->alloc_size *= 2;
		chunk_builder->alloc_size += size;
	}
	memcpy(&chunk_builder->code_buf[chunk_builder->size], ptr, size);
	chunk_builder->size += size;
	return 1;
}

const char chunk_read(struct chunk* chunk) {
	if (chunk->pos == chunk->size)
		return chunk->last_code = MACHINE_END;
	return chunk->last_code = chunk->code[chunk->pos++];
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

const int chunk_write_value(struct chunk_builder* chunk_builder, struct value value) {
	if (value.type == VALUE_TYPE_OBJ)
		return 0;
	struct value copy;
	copy_value(&copy, &value);
	chunk_write_size(chunk_builder, &copy, sizeof(struct value));
	return 1;
}

void chunk_write_chunk(struct chunk_builder* dest, struct chunk src, const int free_chunk) {
	chunk_write_size(dest, src.code, src.size);
	if(free_chunk)
		free(src.code);
}

void chunk_jump_to(struct chunk* chunk, const uint64_t pos) {
	if (pos >= chunk->size) {
		chunk->pos = chunk->size;
		chunk->last_code = MACHINE_END;
	}
	chunk->pos = pos;
}

static void chunk_skip_ins(struct chunk* chunk) {
	char op_code = chunk->last_code;
	switch (op_code)
	{
	case MACHINE_CALL_EXTERN:
	case MACHINE_INHERIT_REC:
		chunk_read_size(chunk, sizeof(uint64_t));
	case MACHINE_LABEL:
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
		chunk_read(chunk);
		break;
	case MACHINE_BUILD_PROTO: {
		chunk_read_ulong(chunk);
		uint64_t i = chunk_read_ulong(chunk);
		while (i--)
			chunk_read_ulong(chunk);
		break;
	}
	}
	chunk_read(chunk);
}

void chunk_skip(struct chunk* chunk, uint64_t depth) {
	while (chunk->last_code != MACHINE_END)
	{
		char op_code = chunk->last_code;
		if (op_code == MACHINE_FLAG_SKIP || op_code == MACHINE_COND_SKIP || op_code == MACHINE_LABEL)
			depth++;
		else if (op_code == MACHINE_END_SKIP)
			depth--;
		if (depth == 0)
			break;
		chunk_skip_ins(chunk);
	}
}