#pragma once

#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>
#include "value.h"
#include "opcodes.h"
#include "operators.h"

struct chunk_builder {
	uint8_t* code_buf;
	uint64_t size;
	uint64_t alloc_size;
};

struct chunk {
	enum op_code last_code;
	uint8_t* code;
	uint64_t size;
	uint64_t pos;
};

void init_chunk(struct chunk* chunk, uint8_t* code, const uint64_t size);
void free_chunk(struct chunk* chunk);

const int init_chunk_builder(struct chunk_builder* builder);
struct chunk build_chunk(struct chunk_builder* builder);

const int chunk_write_ulong(struct chunk_builder* chunk_builder, const uint64_t ulong);
const int chunk_write_value(struct chunk_builder* chunk_builder, struct value value);
const int chunk_write_chunk(struct chunk_builder* dest, struct chunk src, const int free_chunk);
const int chunk_write_opcode(struct chunk_builder* chunk_builder, enum op_code opcode);
const int chunk_write_bin_op(struct chunk_builder* chunk_builder, enum binary_operator bin_op);
const int chunk_write_uni_op(struct chunk_builder* chunk_builder, enum unary_operator uni_op);

const void* chunk_write_size(struct chunk_builder* chunk_builder, const void* ptr, const uint64_t size);

const uint64_t chunk_read_ulong(struct chunk* chunk);
const struct value chunk_read_value(struct chunk* chunk);
const enum op_code chunk_read_opcode(struct chunk* chunk);
const enum binary_operator chunk_read_bin_op(struct chunk* chunk);
const enum binary_operator chunk_read_uni_op(struct chunk* chunk);

const void* chunk_read_size(struct chunk* chunk, const uint64_t size);

void chunk_jump_to(struct chunk* chunk, const uint64_t pos);
const int chunk_optimize(struct chunk* chunk, uint64_t offset);
#endif // !CHUNK_H
 