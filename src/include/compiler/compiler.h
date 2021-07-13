#pragma once

#ifndef COMPILER_H
#define COMPILER_H

#include <stdint.h>
#include "../error.h"
#include "../hash.h"
#include "scanner.h"
#include "chunk.h"

struct compiler
{
	struct scanner scanner;
	struct token last_tok;
	struct chunk_builder chunk_builder;
	enum error last_err;

	uint64_t imported_file_hashes[255];
	uint_fast8_t imported_files;
};

//initializes a compiler instance
void init_compiler(struct compiler* compiler, const char* source);

//compiles a program, and stores the output in "chunk builder"
const int compile(struct compiler* compiler, const int repl_mode);

//reads a token as a compiler
struct token compiler_read_tok(struct compiler* compiler);

//compiles an expression using shunting-yard
const int compile_expression(struct compiler* compiler, enum op_precedence min_prec, const int expr_optimize);

//compiles a block of code
const int compile_body(struct compiler* compiler, const int func_encapsulated);

#endif // !COMPILER_H