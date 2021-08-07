#pragma once

#ifndef COMPILER_H
#define COMPILER_H

#include <stdint.h>
#include "error.h"
#include "hash.h"
#include "scanner.h"
#include "machine.h"
#include "chunk.h"
#include "debug.h"

struct compiler
{
	struct scanner scanner;
	struct token last_tok;
	enum error last_err;

	struct chunk_builder data_builder, code_builder;
	struct machine* machine;

	uint64_t imported_file_hashes[255];
	uint_fast8_t imported_files;

	const char* include_dir;
	size_t include_dir_len;
};

//initializes a compiler instance
void init_compiler(struct compiler* compiler, struct machine* machine, const char* include_dir, const char* source, const char* file);

//compiles a program, and stores the output in "chunk builder"
const int compile(struct compiler* compiler, struct loc_table* loc_table, const int repl_mode);

//reads a token as a compiler
struct token compiler_read_tok(struct compiler* compiler);

//compiles an expression using shunting-yard
const int compile_expression(struct compiler* compiler, struct chunk_builder* builder, enum op_precedence min_prec, const int optimize_copy, uint64_t optimize_goto);

//compiles a block of code
const int compile_body(struct compiler* compiler, struct chunk_builder* builder, struct loc_table* loc_table, uint64_t callee, uint64_t proc_encapsulated);

struct chunk compiler_get_chunk(struct compiler* compiler, uint64_t prev_offset);

#endif // !COMPILER_H