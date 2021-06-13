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
			scanner->last_err = error_unrecognized_control_seq;
		}
	}
	return c;
}

const int read_str(struct scanner* scanner, char* str, const int data_mode) {
	unsigned int len = 0;
	while (scanner->last_char == '\t' || scanner->last_char == '\r' || scanner->last_char == ' ' || scanner->last_char == '\n')
		read_char(scanner);
	if (scanner->last_char != '\"') {
		scanner->last_err = error_unexpected_char;
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

struct token read_tok(struct scanner* scanner) {
	while (scanner->last_char == '\t' || scanner->last_char == '\r' || scanner->last_char == ' ' || scanner->last_char == '\n')
		read_char(scanner);
	struct token tok;
	const char* start = &scanner->source[scanner->pos - 1];
	unsigned long length = 0;
	if (isalpha(scanner->last_char)) {
		while (isalpha(scanner->last_char) || scanner->last_char == '_') {
			read_char(scanner);
			length++;
		}
		unsigned long id_hash = hash(start, length);
		switch (id_hash)
		{
		case 5863476: //ifs
			tok.type = tok_if;
			break;
		case 2090224421: //elif
			tok.type = tok_elif;
			break;
		case 2090224750: //else
			tok.type = tok_else;
			break;
		case 279132286: //while
			tok.type = tok_while;
			break;
		case 2090626457: //proc
			tok.type = tok_proc;
			break;
		case 421984292:
			tok.type = tok_record;
			break;
		case 193500239:
			tok.type = tok_new;
			break;
		case 193505681: //set
			tok.type = tok_set;
			break;
		case 5863848: //to
			tok.type = tok_to;
			break;
		case 193504578://ref
			tok.type = tok_ref;
			break;
		case 3824527: //goproc
			tok.type = tok_goto;
			break;
		case 422601765: //return
			tok.type = tok_return;
			break;
		case 2654384009: //include
			tok.type = tok_include;
			break;
		case 253189136: //alloc
			tok.type = tok_alloc;
			break;
		case 4231324027: //exterm
			tok.type = tok_extern;
			break;
		case 193486360: //and
			tok.type = tok_bin_op;
			tok.payload.bin_op = operator_and;
			break;
		case 5863686: //or
			tok.type = tok_bin_op;
			tok.payload.bin_op = operator_or;
			break;
		case 193495071: //inc
			tok.type = tok_uni_op;
			tok.payload.uni_op = operator_increment;
			break;
		case 193489329: //dec
			tok.type = tok_uni_op;
			tok.payload.uni_op = operator_decriment;
			break;
		case 2090557760: //null
			tok.type = tok_primative;
			init_null_value(&tok.payload.primative);
			break;
		case 2090770405: //true
			tok.type = tok_primative;
			init_num_value(&tok.payload.primative, 1);
			break;
		case 258723568: //false
			tok.type = tok_primative;
			init_num_value(&tok.payload.primative, 0);
			break;
		default:
			tok.type = tok_identifier;
			tok.payload.identifier = id_hash;
		}
	}
	else if (isalnum(scanner->last_char)) {
		while (isalnum(scanner->last_char) || scanner->last_char == '.') {
			length++;
			read_char(scanner);
		}
		tok.type = tok_primative;
		init_num_value(&tok.payload.primative, strtod(start, NULL));
	}
	else if (scanner->last_char == '\'') {
		read_char(scanner);
		init_char_value(&tok.payload.primative, read_data_char(scanner));
		if (scanner->last_err == error_unrecognized_control_seq) {
			free_value(&tok.payload.primative);
			tok.type = tok_error;
			return tok;
		}
		if (scanner->last_char != '\'') {
			free_value(&tok.payload.primative);
			scanner->last_err = error_unexpected_char;
			tok.type = tok_error;
			return tok;
		}
		tok.type = tok_primative;
		read_char(scanner);
	}
	else {
		switch (scanner->last_char)
		{
		case ',':
			tok.type = tok_comma;
			break;
		case '.':
			tok.type = tok_period;
			break;
		case '[': 
			tok.type = tok_open_bracket;
			break;
		case ']':
			tok.type = tok_close_bracket;
			break;
		case '(':
			tok.type = tok_open_paren;
			break;
		case ')':
			tok.type = tok_close_paren;
			break;
		case '{':
			tok.type = tok_open_brace;
			break;
		case '}':
			tok.type = tok_close_brace;
			break;
		case '+':
			tok.type = tok_bin_op;
			tok.payload.bin_op = operator_add;
			break;
		case '-':
			tok.type = tok_bin_op;
			tok.payload.bin_op = operator_subtract;
			break;
		case '*':
			tok.type = tok_bin_op;
			tok.payload.bin_op = operator_multiply;
			break;
		case '/':
			tok.type = tok_bin_op;
			tok.payload.bin_op = operator_divide;
			break;
		case '%':
			tok.type = tok_bin_op;
			tok.payload.bin_op = operator_modulo;
			break;
		case '^':
			tok.type = tok_bin_op;
			tok.payload.bin_op = operator_power;
			break;
		case '=':
			if (read_char(scanner) != '=') {
				tok.type = tok_error;
				scanner->last_err = error_unexpected_char;
			}
			else {
				tok.type = tok_bin_op;
				tok.payload.bin_op = operator_equals;
			}
			break;
		case '!':
			if (peek_char(scanner) == '=') {
				tok.type = tok_bin_op;
				tok.payload.bin_op = operator_not_equal;
				read_char(scanner);
			}
			else {
				tok.type = tok_uni_op;
				tok.payload.uni_op = operator_invert;
			}
			break;
		case '>':
			tok.type = tok_bin_op;
			if (peek_char(scanner) == '=') {
				tok.payload.bin_op = operator_more_equal;
				read_char(scanner);
			}
			else
				tok.payload.bin_op = operator_more;
			break;
		case '<':
			tok.type = tok_bin_op;
			if (peek_char(scanner) == '=') {
				tok.payload.bin_op = operator_less_equal;
				read_char(scanner);
			}
			else
				tok.payload.bin_op = operator_less;
			break;
		case '\"':
			tok.type = tok_str;
			return tok;
		case 0:
			tok.type = tok_end;
			break;
		default:
			tok.type = tok_error;
			scanner->last_err = error_unrecognized_tok;
		}
		read_char(scanner);
	}
	return tok;
}