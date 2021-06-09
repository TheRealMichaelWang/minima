#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "hash.h"
#include "error.h"
#include "scanner.h"

const char read_char(struct scanner* scanner) {
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

const char peek_char(struct scanner* scanner) {
	if (scanner->pos == scanner->size)
		return 0;
	return scanner->source[scanner->pos];
}

const char read_data_char(struct scanner* scanner)
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
		case '\'':
		case '\"':
		case '\\': {
			char toret = scanner->last_char;
			read_char(scanner);
			return toret; 
		}
		default:
			scanner->last_err = error_unrecognized_control_seq;
		}
	}
	return c;
}

const int read_str(struct scanner* scanner, char* str) {
	unsigned int len = 0;
	while (scanner->last_char == '\t' || scanner->last_char == '\r' || scanner->last_char == ' ' || scanner->last_char == '\n')
		read_char(scanner);
	if (scanner->last_char != '\"') {
		scanner->last_err = error_unexpected_char;
		return 0;
	}
	while (read_char(scanner) != '\"')
		str[len++] = scanner->last_char;
	str[len] = 0;
	read_char(scanner);
	return 1;
}

struct token read_tok(struct scanner* scanner) {
	while (scanner->last_char == '\t' || scanner->last_char == '\r' || scanner->last_char == ' ' || scanner->last_char == '\n')
		read_char(scanner);
	struct token tok;
	const char* start = &scanner->source[scanner->pos - 1];
	unsigned long length = 0;
	if (isalpha(scanner->last_char)) {
		while (isalpha(scanner->last_char)) {
			read_char(scanner);
			length++;
		}
		unsigned long id_hash = hash(start, length);
		switch (id_hash)
		{
		case 5863476: //ifs
			tok.type = keyword_if;
			break;
		case 2090224421: //elif
			tok.type = keyword_elif;
			break;
		case 2090224750: //else
			tok.type = keyword_else;
			break;
		case 279132286: //while
			tok.type = keyword_while;
			break;
		case 2090626457: //proc
			tok.type = define_procedure;
			break;
		case 193505681: //set
			tok.type = keyword_set;
			break;
		case 5863848: //to
			tok.type = keyword_to;
			break;
		case 193504578://ref
			tok.type = keyword_ref;
			break;
		case 3824527: //goproc
			tok.type = goto_procedure;
			break;
		case 422601765: //return
			tok.type = return_procedure;
			break;
		case 2654384009: //include
			tok.type = keyword_include;
			break;
		case 253189136: //alloc
			tok.type = dynamic_alloc;
			break;
		case 4231324027: //exterm
			tok.type = goto_extern;
			break;
		case 193486360: //and
			tok.type = binary_op;
			tok.payload.bin_op = operator_and;
			break;
		case 5863686: //or
			tok.type = binary_op;
			tok.payload.bin_op = operator_or;
			break;
		case 193495071: //inc
			tok.type = unary_op;
			tok.payload.uni_op = operator_increment;
			break;
		case 193489329: //dec
			tok.type = unary_op;
			tok.payload.uni_op = operator_decriment;
			break;
		case 2090557760: //null
			tok.type = primative;
			init_null(&tok.payload.primative);
			break;
		case 2090770405: //true
			tok.type = primative;
			init_num(&tok.payload.primative, 1);
			break;
		case 258723568: //false
			tok.type = primative;
			init_num(&tok.payload.primative, 0);
			break;
		default:
			tok.type = identifier;
			tok.payload.identifier = id_hash;
		}
	}
	else if (isalnum(scanner->last_char)) {
		while (isalnum(scanner->last_char) || scanner->last_char == '.') {
			length++;
			read_char(scanner);
		}
		tok.type = primative;
		init_num(&tok.payload.primative, strtod(start, NULL));
	}
	else if (scanner->last_char == '\'') {
		read_char(scanner);
		init_char(&tok.payload.primative, read_data_char(scanner));
		if (scanner->last_err == error_unrecognized_control_seq) {
			free_value(&tok.payload.primative);
			tok.type = error;
		}
		if (tok.type != error && read_char(scanner) != '\'') {
			free_value(&tok.payload.primative);
			scanner->last_err = error_unexpected_char;
			tok.type = error;
		}
		tok.type = primative;
	}
	else if (scanner->last_char == '\"') {
		tok.type = primative;
		read_char(scanner);
		char* buffer = NULL;
		while (scanner->last_char != '\"')
		{
			buffer = realloc(buffer, ++length);
			if (buffer == NULL) {
				tok.type = error;
				scanner->last_err = error_insufficient_memory;
				break;
			}
			buffer[length - 1] = read_data_char(scanner);
			if (scanner->last_err == error_unrecognized_control_seq) {
				tok.type = error;
				break;
			}
		}
		read_char(scanner);
		struct collection* col = malloc(sizeof(struct collection));
		init_collection(col, length);
		for (unsigned long i = 0; i < length; i++) {
			col->inner_collection[i] = malloc(sizeof(struct value));
			init_char(col->inner_collection[i], buffer[i]);
		}
		init_col(&tok.payload.primative, col);
		free(buffer);
	}
	else {
		switch (scanner->last_char)
		{
		case ',':
			tok.type = comma;
			break;
		case '[': 
			tok.type = open_bracket;
			break;
		case ']':
			tok.type = close_bracket;
			break;
			break;
		case '(':
			tok.type = open_paren;
			break;
		case ')':
			tok.type = close_paren;
			break;
		case '{':
			tok.type = open_brace;
			break;
		case '}':
			tok.type = close_brace;
			break;
		case '+':
			tok.type = binary_op;
			tok.payload.bin_op = operator_add;
			break;
		case '-':
			tok.type = binary_op;
			tok.payload.bin_op = operator_subtract;
			break;
		case '*':
			tok.type = binary_op;
			tok.payload.bin_op = operator_multiply;
			break;
		case '/':
			tok.type = binary_op;
			tok.payload.bin_op = operator_divide;
			break;
		case '%':
			tok.type = binary_op;
			tok.payload.bin_op = operator_modulo;
			break;
		case '^':
			tok.type = binary_op;
			tok.payload.bin_op = operator_power;
			break;
		case '=':
			if (read_char(scanner) != '=') {
				tok.type = error;
				scanner->last_err = error_unexpected_char;
			}
			else {
				tok.type = binary_op;
				tok.payload.bin_op = operator_equals;
			}
			break;
		case '!':
			if (peek_char(scanner) == '=') {
				tok.type = binary_op;
				tok.payload.bin_op = operator_not_equal;
				read_char(scanner);
			}
			else {
				tok.type = unary_op;
				tok.payload.uni_op = operator_invert;
			}
			break;
		case '>':
			tok.type = binary_op;
			if (peek_char(scanner) == '=') {
				tok.payload.bin_op = operator_more_equal;
				read_char(scanner);
			}
			else
				tok.payload.bin_op = operator_more;
			break;
		case '<':
			tok.type = binary_op;
			if (peek_char(scanner) == '=') {
				tok.payload.bin_op = operator_less_equal;
				read_char(scanner);
			}
			else
				tok.payload.bin_op = operator_less;
			break;
		case 0:
			tok.type = end;
			break;
		default:
			tok.type = error;
			scanner->last_err = error_unrecognized_tok;
		}
		read_char(scanner);
	}
	return tok;
}