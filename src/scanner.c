#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "include/hash.h"
#include "include/error.h"
#include "include/compiler/scanner.h"

static const char read_char(struct scanner* scanner) {
	if (scanner->pos == scanner->size)
		return scanner->last_char = 0;
	return scanner->last_char = scanner->source[scanner->pos++];
}

void init_scanner(struct scanner* scanner, const char* source) {
	scanner->source = source;
	scanner->pos = 0;
	scanner->size = strlen(source);
	read_char(scanner);
}

static const char peek_char(struct scanner* scanner) {
	if (scanner->pos == scanner->size)
		return 0;
	return scanner->source[scanner->pos];
}

static const char read_data_char(struct scanner* scanner)
{
	char c = scanner->last_char;
	read_char(scanner);
	if (c == '\\') {
		switch (scanner->last_char)
		{
		case 'b':
			read_char(scanner);
			return '\b';
		case 't':
			read_char(scanner);
			return '\t';
		case 'r':
			read_char(scanner);
			return '\r';
		case 'n':
			read_char(scanner);
			return '\n';
		case '0':
			read_char(scanner);
			return 0;
		case '\'':
		case '\"':
		case '\\': {
			char toret = scanner->last_char;
			read_char(scanner);
			return toret; 
		}
		default:
			scanner->last_err = ERROR_UNRECOGNIZED_CONTROL_SEQ;
		}
	}
	return c;
}

const int scanner_read_str(struct scanner* scanner, char* str, const int data_mode) {
	uint32_t len = 0;
	while (scanner->last_char == '\t' || scanner->last_char == '\r' || scanner->last_char == ' ' || scanner->last_char == '\n')
		read_char(scanner);
	if (scanner->last_char != '\"') {
		scanner->last_err = ERROR_UNEXPECTED_CHAR;
		return 0;
	}
	if (data_mode) {
		read_char(scanner);
		while (scanner->last_char != '\"')
			str[len++] = read_data_char(scanner);
	}
	else {
		while (read_char(scanner) != '\"')
			str[len++] = scanner->last_char;
	}
	str[len] = 0;
	read_char(scanner);
	return 1;
}

struct token scanner_read_tok(struct scanner* scanner) {
	while (scanner->last_char == '\t' || scanner->last_char == '\r' || scanner->last_char == ' ' || scanner->last_char == '\n')
		read_char(scanner);
	struct token tok;
	const char* start = &scanner->source[scanner->pos - 1];
	uint64_t length = 0;
	if (isalpha(scanner->last_char)) {
		while (isalpha(scanner->last_char) || isalnum(scanner->last_char) || scanner->last_char == '_') {
			read_char(scanner);
			length++;
		}
		uint64_t id_hash = hash(start, length);
		switch (id_hash)
		{
		case 5863476: //ifs
			tok.type = TOK_IF;
			break;
		case 2090224421: //elif
			tok.type = TOK_ELIF;
			break;
		case 2090224750: //else
			tok.type = TOK_ELSE;
			break;
		case 279132286: //while
			tok.type = TOK_WHILE;
			break;
		case 2090626457: //proc
			tok.type = TOK_PROC;
			break;
		case 421984292:
			tok.type = TOK_RECORD;
			break;
		case 5863225:
			tok.type = TOK_AS;
			break;
		case 193500239:
			tok.type = TOK_NEW;
			break;
		case 193505681: //set
			tok.type = TOK_SET;
			break;
		case 5863848: //to
			tok.type = TOK_TO;
			break;
		case 193504578://ref
			tok.type = TOK_REF;
			break;
		case 3824527: //goproc
			tok.type = TOK_GOTO;
			break;
		case 422601765: //return
			tok.type = TOK_RETURN;
			break;
		case 2654384009: //include
			tok.type = TOK_INCLUDE;
			break;
		case 253189136: //alloc
			tok.type = TOK_ALLOC;
			break;
		case 4231324027: //exterm
			tok.type = TOK_EXTERN;
			break;
		case 2194734848: //extend
			tok.type = TOK_EXTEND;
			break;
		case 193486360: //and
			tok.type = TOK_BINARY_OP;
			tok.payload.bin_op = OPERATOR_AND;
			break;
		case 5863686: //or
			tok.type = TOK_BINARY_OP;
			tok.payload.bin_op = OPERATOR_OR;
			break;
		case 193495071: //inc
			tok.type = TOK_UNARY_OP;
			tok.payload.uni_op = OPERATOR_INCREMENT;
			break;
		case 193489329: //dec
			tok.type = TOK_UNARY_OP;
			tok.payload.uni_op = OPERATOR_DECRIMENT;
			break;
		case 2090557760: //null
			tok.type = TOK_PRIMATIVE;
			tok.payload.primative = const_value_null;
			break;
		case 2090770405: //true
			tok.type = TOK_PRIMATIVE;
			tok.payload.primative = const_value_true;
			break;
		case 258723568: //false
			tok.type = TOK_PRIMATIVE;
			tok.payload.primative = const_value_false;
			break;
		case 193504585: //remark
			while (scanner->last_char != '\n')
				read_char(scanner);
			read_char(scanner);
			tok.type = TOK_REMARK;
			break;
		default:
			tok.type = TOK_IDENTIFIER;
			tok.payload.identifier = id_hash;
		}
	}
	else if (isalnum(scanner->last_char)) {
		while (isalnum(scanner->last_char) || scanner->last_char == '.') {
			length++;
			read_char(scanner);
		}
		tok.type = TOK_PRIMATIVE;
		tok.payload.primative = NUM_VALUE(strtod(start, NULL));
	}
	else if (scanner->last_char == '\'') {
		read_char(scanner);
		tok.payload.primative = CHAR_VALUE(read_data_char(scanner));
		if (scanner->last_err == ERROR_UNRECOGNIZED_CONTROL_SEQ) {
			tok.type = TOK_ERROR;
			return tok;
		}
		if (scanner->last_char != '\'') {
			scanner->last_err = ERROR_UNEXPECTED_CHAR;
			tok.type = TOK_ERROR;
			return tok;
		}
		tok.type = TOK_PRIMATIVE;
		read_char(scanner);
	}
	else {
		switch (scanner->last_char)
		{
		case ',':
			tok.type = TOK_COMMA;
			break;
		case '.':
			tok.type = TOK_PERIOD;
			break;
		case '[': 
			tok.type = TOK_OPEN_BRACKET;
			break;
		case ']':
			tok.type = TOK_CLOSE_BRACKET;
			break;
		case '(':
			tok.type = TOK_OPEN_PAREN;
			break;
		case ')':
			tok.type = TOK_CLOSE_PAREN;
			break;
		case '{':
			tok.type = TOK_OPEN_BRACE;
			break;
		case '}':
			tok.type = TOK_CLOSE_BRACE;
			break;
		case '#':
			tok.type = TOK_HASHTAG;
			break;
		case '+':
			tok.type = TOK_BINARY_OP;
			tok.payload.bin_op = OPERATOR_ADD;
			break;
		case '-':
			tok.type = TOK_BINARY_OP;
			tok.payload.bin_op = OPERATOR_SUBTRACT;
			break;
		case '*':
			tok.type = TOK_BINARY_OP;
			tok.payload.bin_op = OPERATOR_MULTIPLY;
			break;
		case '/':
			tok.type = TOK_BINARY_OP;
			tok.payload.bin_op = OPERATOR_DIVIDE;
			break;
		case '%':
			tok.type = TOK_BINARY_OP;
			tok.payload.bin_op = OPERATOR_MODULO;
			break;
		case '^':
			tok.type = TOK_BINARY_OP;
			tok.payload.bin_op = OPERATOR_POWER;
			break;
		case '=':
			if (read_char(scanner) != '=') {
				tok.type = TOK_ERROR;
				scanner->last_err = ERROR_UNEXPECTED_CHAR;
			}
			else {
				tok.type = TOK_BINARY_OP;
				tok.payload.bin_op = OPERATOR_EQUALS;
			}
			break;
		case '!':
			if (peek_char(scanner) == '=') {
				tok.type = TOK_BINARY_OP;
				tok.payload.bin_op = OPERATOR_NOT_EQUAL;
				read_char(scanner);
			}
			else {
				tok.type = TOK_UNARY_OP;
				tok.payload.uni_op = OPERATOR_INVERT;
			}
			break;
		case '>':
			tok.type = TOK_BINARY_OP;
			if (peek_char(scanner) == '=') {
				tok.payload.bin_op = OPERATOR_MORE_EQUAL;
				read_char(scanner);
			}
			else
				tok.payload.bin_op = OPERATOR_MORE;
			break;
		case '<':
			tok.type = TOK_BINARY_OP;
			if (peek_char(scanner) == '=') {
				tok.payload.bin_op = OPERATOR_LESS_EQUAL;
				read_char(scanner);
			}
			else
				tok.payload.bin_op = OPERATOR_LESS;
			break;
		case '\"':
			tok.type = TOK_STR;
			return tok;
		case 0:
			tok.type = TOK_END;
			break;
		default:
			tok.type = TOK_ERROR;
			scanner->last_err = ERROR_UNRECOGNIZED_TOKEN;
		}
		read_char(scanner);
	}
	return tok;
}