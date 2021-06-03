#include <stdlib.h>
#include <string.h>
#include "chunk.h"

void init_chunk(struct chunk* chunk, char* code, const unsigned long size) {
	chunk->pos = 0;
	chunk->size = size;
	chunk->code = code;
	chunk->last_code = 66;
	//read(chunk);
}

void free_chunk(struct chunk* chunk) {
	free(chunk->code);
}

void init_chunk_builder(struct chunk_builder* builder) {
	builder->code_buf = malloc(100 * sizeof(char));
	builder->size = 0;
	builder->alloc_size = 100;
}

struct chunk build_and_free(struct chunk_builder* builder) {
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

int write_value(struct chunk_builder* chunk_builder, struct value value) {
	struct value copy;
	if (!copy_value(&copy, &value))
		return 0;
	write_size(chunk_builder, &copy, sizeof(value));
	return 1;
}

void jump_to(struct chunk* chunk, const unsigned long pos) {
	if (pos >= chunk->size) {
		chunk->pos = chunk->size;
		chunk->last_code = 0;
	}
	chunk->pos = pos;
}