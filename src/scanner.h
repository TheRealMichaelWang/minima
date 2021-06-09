#pragma once

#ifndef SCANNER_H
#define SCANNER_H

#include "operators.h"
#include "value.h"

struct scanner {
	const char* source;
	unsigned long size;
	unsigned long pos;

	char last_char;
	char last_err;
};

struct token {
	enum token_type {
		keyword_if,
		keyword_else,
		keyword_elif,
		keyword_while,
		define_procedure,
		keyword_set,
		keyword_to,
		keyword_ref,
		keyword_include,
		goto_procedure,
		goto_extern,
		return_procedure,
		dynamic_alloc,
		identifier,
		primative,
		binary_op,
		unary_op,
		open_paren,
		close_paren,
		open_brace,
		close_brace,
		open_bracket,
		close_bracket,
		comma,
		end,
		error
	} type;

	union token_type_payload
	{
		enum binary_operator bin_op;
		enum unary_operator uni_op;

		struct value primative;

		unsigned long identifier;
	} payload;
};

void init_scanner(struct scanner* scanner, const char* source);

struct token read_tok(struct scanner* scanner);

const int read_str(struct scanner* scanner, char* str);

#endif // !SCANNER_H
