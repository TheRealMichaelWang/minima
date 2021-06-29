#pragma once

#ifndef SCANNER_H
#define SCANNER_H

#include <stdint.h>
#include "tokens.h"

struct scanner {
	const char* source;
	uint64_t size;
	uint64_t pos;

	char last_char;
	char last_err;
};

void init_scanner(struct scanner* scanner, const char* source);

struct token scanner_read_tok(struct scanner* scanner);

const int scanner_read_str(struct scanner* scanner, char* str, const int data_mode);

#endif // !SCANNER_H
