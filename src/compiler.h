#pragma once

#ifndef COMPILER_H
#define COMPILER_H

#include "error.h"
#include "scanner.h"
#include "machine.h"
#include "chunk.h"

struct compiler
{
	struct scanner scanner;
	struct token last_tok;
	struct chunk_builder chunk_builder;
	enum error last_err;
};

void init_compiler(struct compiler* compiler, const char* source);
const int compile(struct compiler* compiler, const int repl_mode);

#endif // !COMPILER_H