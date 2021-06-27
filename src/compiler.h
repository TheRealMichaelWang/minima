#pragma once

#ifndef COMPILER_H
#define COMPILER_H

#include <stdint.h>
#include "error.h"
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

#endif // !COMPILER_H