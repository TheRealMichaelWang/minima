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
struct chunk build_and_free(struct chunk_builder* builder);

int write(struct chunk_builder* chunk_builder, const char code);
int write_size(struct chunk_builder* chunk_builder, const void* ptr, const unsigned long size);

const char read(struct chunk* chunk);
const void* read_size(struct chunk* chunk, const unsigned long size);

inline const struct value* read_value(const struct chunk* chunk) {
	return read_size(chunk, sizeof(struct value));
}

int write_value(struct chunk_builder* chunk_builder, struct value value);

void jump_to(struct chunk* chunk, const unsigned long pos);

inline void write_ulong(struct chunk_builder* chunk_builder, const unsigned long ulong) {
	write_size(chunk_builder, &ulong, sizeof(ulong));
}

inline const unsigned long read_ulong(const struct chunk* chunk) {
	return *(unsigned long*)read_size(chunk, sizeof(unsigned long));
}

inline const int end(const struct chunk* chunk) {
	return chunk->pos == chunk->size;
}

#endif // !CHUNK_H
