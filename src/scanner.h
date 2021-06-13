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
		tok_if,
		tok_else,
		tok_elif,
		tok_while,
		tok_proc,
		tok_record,
		tok_set,
		tok_to,
		tok_as,
		tok_ref,
		tok_new,
		tok_include,
		tok_goto,
		tok_extern,
		tok_return,
		tok_alloc,
		tok_identifier,
		tok_primative,
		tok_bin_op,
		tok_uni_op,
		tok_open_paren,
		tok_close_paren,
		tok_open_brace,
		tok_close_brace,
		tok_open_bracket,
		tok_close_bracket,
		tok_comma,
		tok_period,
		tok_end,
		tok_str,
		tok_error
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

const int read_str(struct scanner* scanner, char* str, const int data_mode);

#endif // !SCANNER_H
