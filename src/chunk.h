#pragma once

#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>
#include "value.h"

struct chunk_builder {
	char* code_buf;
	uint64_t size;
	uint64_t alloc_size;
};

struct chunk {
	char last_code;
	char* code;
	uint64_t size;
	uint64_t pos;
};

void init_chunk(struct chunk* chunk, char* code, const uint64_t size);
void free_chunk(struct chunk* chunk);

void init_chunk_builder(struct chunk_builder* builder);
struct chunk build_chunk(struct chunk_builder* builder);

const int chunk_write_value(struct chunk_builder* chunk_builder, struct value value);
void chunk_write_chunk(struct chunk_builder* dest, struct chunk src, const int free_chunk);
const int chunk_write(struct chunk_builder* chunk_builder, const char code);
const int chunk_write_size(struct chunk_builder* chunk_builder, const void* ptr, const uint64_t size);

inline void chunk_write_ulong(struct chunk_builder* chunk_builder, const uint64_t ulong) {
	chunk_write_size(chunk_builder, &ulong, sizeof(ulong));
}

const char chunk_read(struct chunk* chunk);
const void* chunk_read_size(struct chunk* chunk, const uint64_t size);

inline const uint64_t chunk_read_ulong(struct chunk* chunk) {
	return *(uint64_t*)chunk_read_size(chunk, sizeof(uint64_t));
}

inline const struct value* chunk_read_value(struct chunk* chunk) {
	return chunk_read_size(chunk, sizeof(struct value));
}

void chunk_jump_to(struct chunk* chunk, const uint64_t pos);

void chunk_skip_ins(struct chunk* chunk);
void chunk_skip(struct chunk* chunk, uint64_t depth);

#endif // !CHUNK_H
