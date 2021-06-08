#pragma once

#ifndef CHUNK_H
#define CHUNK_H

#include "value.h"

struct chunk_builder {
	char* code_buf;
	unsigned long size;
	unsigned long alloc_size;
};

struct chunk {
	char last_code;
	char* code;
	unsigned long size;
	unsigned long pos;
};

void init_chunk(struct chunk* chunk, char* code, const unsigned long size);
void free_chunk(struct chunk* chunk);

void init_chunk_builder(struct chunk_builder* builder);
struct chunk build_chunk(struct chunk_builder* builder);

int write(struct chunk_builder* chunk_builder, const char code);
int write_size(struct chunk_builder* chunk_builder, const void* ptr, const unsigned long size);

const char read(struct chunk* chunk);
const void* read_size(struct chunk* chunk, const unsigned long size);

inline const struct value* read_value(struct chunk* chunk) {
	return read_size(chunk, sizeof(struct value));
}

int write_value(struct chunk_builder* chunk_builder, struct value value);

void write_chunk(struct chunk_builder* dest, struct chunk src);

void jump_to(struct chunk* chunk, const unsigned long pos);

void skip_instruction(struct chunk* chunk);
void skip(struct chunk* chunk, unsigned long depth);

inline void write_ulong(struct chunk_builder* chunk_builder, const unsigned long ulong) {
	write_size(chunk_builder, &ulong, sizeof(ulong));
}

inline const unsigned long read_ulong(struct chunk* chunk) {
	return *(unsigned long*)read_size(chunk, sizeof(unsigned long));
}

inline const int end_chunk(const struct chunk* chunk) {
	return chunk->pos == chunk->size;
}

#endif // !CHUNK_H
