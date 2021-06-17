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
		TOK_IF,
		TOK_ELSE,
		TOK_ELIF,
		TOK_WHILE,
		TOK_PROC,
		TOK_RECORD,
		TOK_SET,
		TOK_TO,
		TOK_AS,
		TOK_REF,
		TOK_NEW,
		TOK_INCLUDE,
		TOK_GOTO,
		TOK_EXTERN,
		TOK_RETURN,
		TOK_REMARK,
		TOK_ALLOC,
		TOK_IDENTIFIER,
		TOK_PRIMATIVE,
		TOK_BINARY_OP,
		TOK_UNARY_OP,
		TOK_OPEN_PAREN,
		TOK_CLOSE_PAREN,
		TOK_OPEN_BRACE,
		TOK_CLOSE_BRACE,
		TOK_OPEN_BRACKET,
		TOK_CLOSE_BRACKET,
		TOK_COMMA,
		TOK_PERIOD,
		TOK_END,
		TOK_STR,
		TOK_ERROR
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

struct token scanner_read_tok(struct scanner* scanner);

const int scanner_read_str(struct scanner* scanner, char* str, const int data_mode);

#endif // !SCANNER_H
