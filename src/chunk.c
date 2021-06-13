#include <stdlib.h>
#include <string.h>
#include "machine.h"
#include "chunk.h"

void init_chunk(struct chunk* chunk, char* code, const unsigned long size) {
	chunk->pos = 0;
	chunk->size = size;
	chunk->code = code;
	chunk->last_code = 66;
}

void free_chunk(struct chunk* chunk) {
	chunk->pos = 0;
	read(chunk);
	while (chunk->last_code != 0)
	{
		char op_code = chunk->last_code;
		if (op_code == MACHINE_LOAD_CONST) {
 			free_value(read_value(chunk));
			read(chunk);
		}
		else
			skip_instruction(chunk);
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

int write(struct chunk_builder* chunk_builder, const char code) {
	if (chunk_builder->size == chunk_builder->alloc_size) {
		void* new_ptr = realloc(chunk_builder->code_buf, chunk_builder->alloc_size * 2);
		if (new_ptr == NULL)
			return 0;
		chunk_builder->code_buf = new_ptr;
		chunk_builder->alloc_size *= 2;
	}
	chunk_builder->code_buf[chunk_builder->size++] = code;
	return 1;
}


int write_size(struct chunk_builder* chunk_builder, const void* ptr, const unsigned long size) {
	if (chunk_builder->size + size >= chunk_builder->alloc_size) {
		void* new_ptr = realloc(chunk_builder->code_buf, chunk_builder->alloc_size * 2 + size);
		if (new_ptr == NULL)
			return 0;
		chunk_builder->code_buf = new_ptr;
		chunk_builder->alloc_size *= 2;
		chunk_builder->alloc_size += size;
	}
	memcpy(&chunk_builder->code_buf[chunk_builder->size], ptr, size);
	chunk_builder->size += size;
	return 1;
}

const char read(struct chunk* chunk) {
	if (chunk->pos == chunk->size)
		return chunk->last_code = 0;
	return chunk->last_code = chunk->code[chunk->pos++];
}

const void* read_size(struct chunk* chunk, const unsigned long size) {
	if (chunk->pos + size > chunk->size) {
		chunk->last_code = 0;
		return NULL;
	}
	const void* position = &chunk->code[chunk->pos];
	chunk->pos += size;
	//chunk->last_code = chunk->code[chunk->pos-1];
	return position;
}

const int write_value(struct chunk_builder* chunk_builder, struct value value) {
	if (value.type == value_type_object)
		return 0;
	struct value copy;
	copy_value(&copy, &value);
	write_size(chunk_builder, &copy, sizeof(struct value));
	return 1;
}

void write_chunk(struct chunk_builder* dest, struct chunk src) {
	write_size(dest, src.code, src.size);
	free(src.code);
}

void jump_to(struct chunk* chunk, const unsigned long pos) {
	if (pos >= chunk->size) {
		chunk->pos = chunk->size;
		chunk->last_code = 0;
	}
	chunk->pos = pos;
}

void skip_instruction(struct chunk* chunk) {
	char op_code = chunk->last_code;
	switch (op_code)
	{
	case MACHINE_CALL_EXTERN:
		read_size(chunk, sizeof(unsigned long));
	case MACHINE_LABEL:
	case MACHINE_GOTO:
	case MACHINE_GOTO_AS:
	case MACHINE_STORE_VAR:
	case MACHINE_LOAD_VAR:
	case MACHINE_BUILD_COL:
	case MACHINE_SET_PROPERTY:
	case MACHINE_GET_PROPERTY:
	case MACHINE_BUILD_RECORD:
		read_size(chunk, sizeof(unsigned long));
		break;
	case MACHINE_LOAD_CONST:
		read_size(chunk, sizeof(struct value));
		break;
	case MACHINE_EVAL_BIN_OP:
	case MACHINE_EVAL_UNI_OP:
		read(chunk);
		break;
	case MACHINE_BUILD_PROTO: {
		read_ulong(chunk);
		unsigned long i = read_ulong(chunk);
		while (i--)
			read_ulong(chunk);
		break;
	}
	}
	read(chunk);
}

void skip(struct chunk* chunk, unsigned long depth) {
	while (chunk->last_code != 0)
	{
		char op_code = chunk->last_code;
		if (op_code == MACHINE_FLAG_SKIP || op_code == MACHINE_COND_SKIP || op_code == MACHINE_LABEL)
			depth++;
		else if (op_code == MACHINE_END_SKIP)
			depth--;
		if (depth == 0)
			break;
		skip_instruction(chunk);
	}
}