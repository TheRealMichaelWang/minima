#define _CRT_SECURE_NO_DEPRECATE
//buffer security is imoortant, but not as much as portability

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hash.h"
#include "compiler.h"
//#include <cstdio>

enum op_precedence {
	begin,
	compare,
	add,
	multiply,
	exponent
};

enum op_precedence precedence[14] = {
	compare,
	compare,
	compare,
	compare,
	compare,
	compare,
	begin,
	begin,
	add,
	add,
	multiply,
	multiply,
	multiply,
	exponent
};

inline struct token read_ctok(struct compiler* compiler) {
	compiler->last_tok = read_tok(&compiler->scanner);
	if (compiler->last_tok.type == tok_error) {
		compiler->last_err = compiler->scanner.last_err;
	}
	return compiler->last_tok;
}

const int compile_expression(struct compiler* compiler, enum op_precedence min_prec, const int expr_optimize);
const int compile_body(struct compiler* compiler, const int func_encapsulated, int* returned);

const int compile_value(struct compiler* compiler, const int expr_optimize) {
	if (compiler->last_tok.type == tok_primative) {
		write(&compiler->chunk_builder, MACHINE_LOAD_CONST);
		write_value(&compiler->chunk_builder, compiler->last_tok.payload.primative);
		free_value(&compiler->last_tok.payload.primative);
		read_ctok(compiler);
	}
	else if (compiler->last_tok.type == tok_str) {
		char* buffer = malloc(150);
		if(buffer == NULL) {
			compiler->last_err = error_insufficient_memory;
			return 0;
		}
		if (!read_str(&compiler->scanner, buffer, 1)) {
			free(buffer);
			return 0;
		}
		unsigned long len = strlen(buffer);
		for (unsigned long i = 0; i < len; i++) {
			write(&compiler->chunk_builder, MACHINE_LOAD_CONST);
			struct value char_val;
			init_char_value(&char_val, buffer[i]);
			write_value(&compiler->chunk_builder, char_val);
			free_value(&char_val);
		}
		write(&compiler->chunk_builder, MACHINE_BUILD_COL);
		write_ulong(&compiler->chunk_builder, len);
		read_ctok(compiler);
	}
	else if (compiler->last_tok.type == tok_identifier || compiler->last_tok.type == tok_ref) {
		const int is_ref = (compiler->last_tok.type == tok_ref);
		if(is_ref)
			if (read_ctok(compiler).type != tok_identifier) {
				compiler->last_err = error_unexpected_tok;
				return 0;
			}
		write(&compiler->chunk_builder, MACHINE_LOAD_VAR);
		write_ulong(&compiler->chunk_builder, compiler->last_tok.payload.identifier);
		read_ctok(compiler);
		while (compiler->last_tok.type == tok_open_bracket)
		{
			read_ctok(compiler);
			compile_expression(compiler, 0, 1);
			if (compiler->last_tok.type != tok_close_bracket) {
				compiler->last_err = error_unexpected_tok;
				return 0;
			}
			read_ctok(compiler);
			write(&compiler->chunk_builder, MACHINE_GET_INDEX);
		}

		if (!is_ref && !expr_optimize) {
			write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
			write(&compiler->chunk_builder, operator_copy);
		}
	}
	else if (compiler->last_tok.type == tok_open_brace) {
		unsigned long array_size = 0;
		do {
			read_ctok(compiler);
			compile_expression(compiler, 0, 0);
			array_size++;
		} 		while (compiler->last_tok.type == tok_comma);

		if (compiler->last_tok.type != tok_close_brace) {
			compiler->last_err = error_unrecognized_tok;
			return 0;
		}

		write(&compiler->chunk_builder, MACHINE_BUILD_COL);
		write_ulong(&compiler->chunk_builder, array_size);
		read_ctok(compiler);
	}
	else if (compiler->last_tok.type == tok_alloc) {
		if (read_ctok(compiler).type != tok_open_bracket) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}
		read_ctok(compiler);
		if (!compile_expression(compiler, begin, 1))
			return 0;
		write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
		write(&compiler->chunk_builder, operator_alloc);
		if (compiler->last_tok.type != tok_close_bracket) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}
		read_ctok(compiler);
	}
	else if (compiler->last_tok.type == tok_open_paren) {
		read_ctok(compiler);
		compile_expression(compiler, 0, 1);
		if (read_ctok(compiler).type != tok_close_paren) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}
		read_ctok(compiler);
	}
	else if (compiler->last_tok.type == tok_uni_op) {
		enum unary_operator op = compiler->last_tok.payload.uni_op;
		read_ctok(compiler);
		compile_value(compiler, 1);
		write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
		write(&compiler->chunk_builder, op);
	}
	else if (compiler->last_tok.type == tok_goto || compiler->last_tok.type == tok_extern) {
		int is_proc = compiler->last_tok.type == tok_goto;
		if (read_ctok(compiler).type != tok_identifier) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}
		unsigned long proc_id = compiler->last_tok.payload.identifier;
		unsigned long arguments = 0;
		if (read_ctok(compiler).type == tok_open_paren) {
			while (1)
			{
				read_ctok(compiler);
				compile_expression(compiler, begin, 1);
				arguments++;
				if (compiler->last_tok.type != tok_comma)
					break;
			}
			if (compiler->last_tok.type != tok_close_paren) {
				compiler->last_err = error_unexpected_tok;
				return 0;
			}
			read_ctok(compiler);
		}
		if (is_proc) {
			write(&compiler->chunk_builder, MACHINE_GOTO);
			write_ulong(&compiler->chunk_builder, proc_id);
			write(&compiler->chunk_builder, MACHINE_CLEAN);
		}
		else {
			write(&compiler->chunk_builder, MACHINE_CALL_EXTERN);
			write_ulong(&compiler->chunk_builder, proc_id);
			write_ulong(&compiler->chunk_builder, arguments);
		}
	}
	else {
		compiler->last_err = error_unrecognized_tok;
		return 0;
	}
	return 1;
}

const int compile_expression(struct compiler* compiler, enum op_precedence min_prec, const int expr_optimize) {
	char is_id = compiler->last_tok.type == tok_identifier;
	if (!compile_value(compiler, 1)) //lhs
		return 0;
	if (compiler->last_tok.type != tok_bin_op) {
		if (is_id &&!expr_optimize) {
			write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
			write(&compiler->chunk_builder, operator_copy);
		}
		return 1;
	}
	while (compiler->last_tok.type == tok_bin_op && precedence[compiler->last_tok.payload.bin_op] > min_prec)
	{
		enum binary_operator op = compiler->last_tok.payload.bin_op;
		read_ctok(compiler);
		if (!compile_expression(compiler, precedence[op], 1))
			return 0;
		write(&compiler->chunk_builder, MACHINE_EVAL_BIN_OP);
		write(&compiler->chunk_builder, op);
	}
	return 1;
}

const int compile_statement(struct compiler* compiler, const int encapsulated, const int func_encapsulated, int* returned) {
	switch (compiler->last_tok.type)
	{
	case tok_uni_op:
	case tok_extern:
	case tok_goto: {
		if (!compile_value(compiler, 0))
			return 0;
		write(&compiler->chunk_builder, MACHINE_POP);
		break;
	}
	case tok_set: {
		if (read_ctok(compiler).type != tok_identifier) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}
		unsigned long var_id = compiler->last_tok.payload.identifier;
		if (read_ctok(compiler).type == tok_to) {
			read_ctok(compiler);
			if (!compile_expression(compiler, begin, 0))
				return 0;
			write(&compiler->chunk_builder, MACHINE_STORE_VAR);
			write_ulong(&compiler->chunk_builder, var_id);
		}
		else {
			write(&compiler->chunk_builder, MACHINE_LOAD_VAR);
			write_ulong(&compiler->chunk_builder, var_id);
			while (1)
			{
				if (compiler->last_tok.type != tok_open_bracket) {
					compiler->last_err = error_unexpected_tok;
					return 0;
				}
				read_ctok(compiler);
				if (!compile_expression(compiler, begin, 1))
					return 0;
				if (compiler->last_tok.type != tok_close_bracket) {
					compiler->last_err = error_unexpected_tok;
					return 0;
				}
				if (read_ctok(compiler).type == tok_to) {
					read_ctok(compiler);
					if (!compile_expression(compiler, begin, 0))
						return 0;
					write(&compiler->chunk_builder, MACHINE_SET_INDEX);
					break;
				}
				else {
					write(&compiler->chunk_builder, MACHINE_GET_INDEX);
				}
			}
		}
		break;
	}
	case tok_if: {
		write(&compiler->chunk_builder, MACHINE_RESET_FLAG);
		
		read_ctok(compiler);
		if (!compile_expression(compiler, begin, 1))
			return 0;

		write(&compiler->chunk_builder, MACHINE_COND_SKIP);

		if (compiler->last_tok.type != tok_open_brace) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}
		read_ctok(compiler);

		int if_returned = 0;
		if (!compile_body(compiler, func_encapsulated, &if_returned))
			return 0;
		
		write(&compiler->chunk_builder, MACHINE_FLAG);
		write(&compiler->chunk_builder, MACHINE_END_SKIP);

		while (1)
		{
			if (compiler->last_tok.type == tok_elif) {
				read_ctok(compiler);
				write(&compiler->chunk_builder, MACHINE_FLAG_SKIP);
				if (!compile_expression(compiler, begin, 1))
					return 0;
				write(&compiler->chunk_builder, MACHINE_COND_SKIP);

				if (compiler->last_tok.type != tok_open_brace) {
					compiler->last_err = error_unexpected_tok;
					return 0;
				}
				read_ctok(compiler);
				if (!compile_body(compiler, func_encapsulated, &if_returned))
					return 0;

				write(&compiler->chunk_builder, MACHINE_FLAG);
				write(&compiler->chunk_builder, MACHINE_END_SKIP);
				write(&compiler->chunk_builder, MACHINE_END_SKIP);
			}
			else if (compiler->last_tok.type == tok_else) {
				read_ctok(compiler);
				write(&compiler->chunk_builder, MACHINE_FLAG_SKIP);
				if (compiler->last_tok.type != tok_open_brace) {
					compiler->last_err = error_unexpected_tok;
					return 0;
				}
				read_ctok(compiler);
				if (!compile_body(compiler, func_encapsulated, &if_returned))
					return 0;
				write(&compiler->chunk_builder, MACHINE_END_SKIP);
			}
			else
				break;
		}
		break;
	}
	case tok_while: {
		read_ctok(compiler);
		
		struct chunk_builder temp_expr_builder;
		init_chunk_builder(&temp_expr_builder);

		struct chunk_builder og_builder = compiler->chunk_builder;
		compiler->chunk_builder = temp_expr_builder;
		
		if (!compile_expression(compiler, begin, 1))
			return 0;

		temp_expr_builder = compiler->chunk_builder;
		compiler->chunk_builder = og_builder;

		if (compiler->last_tok.type != tok_open_brace) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}
		read_ctok(compiler);
		struct chunk temp_expr_chunk = build_chunk(&temp_expr_builder);

		write_chunk(&compiler->chunk_builder, temp_expr_chunk);
		write(&compiler->chunk_builder, MACHINE_COND_SKIP);

		write(&compiler->chunk_builder, MACHINE_MARK);

		int while_return = 0;
		if (!compile_body(compiler, func_encapsulated, &while_return))
			return 0;
		
		write_chunk(&compiler->chunk_builder, temp_expr_chunk);

		free_chunk(&temp_expr_chunk);
		
		write(&compiler->chunk_builder, MACHINE_COND_RETURN);
		write(&compiler->chunk_builder, MACHINE_END_SKIP);
		break;
	}
	case tok_proc: {
		if (encapsulated) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}

		if (read_ctok(compiler).type != tok_identifier) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}
		unsigned long proc_id = compiler->last_tok.payload.identifier;
		write(&compiler->chunk_builder, MACHINE_LABEL);
		write_ulong(&compiler->chunk_builder, proc_id);
		write(&compiler->chunk_builder, MACHINE_NEW_FRAME);
		
		if (read_ctok(compiler).type == tok_open_paren) {
			unsigned long reverse_buffer[50];
			unsigned int buffer_size = 0;
			while (1)
			{
				if (read_ctok(compiler).type != tok_identifier) {
					compiler->last_err = error_unexpected_tok;
					return 0;
				}
				reverse_buffer[buffer_size++] = compiler->last_tok.payload.identifier;
				if (read_ctok(compiler).type != tok_comma) {
					break;
				}
			}
			if (compiler->last_tok.type != tok_close_paren) {
				compiler->last_err = error_unexpected_tok;
				return 0;
			}

			while (buffer_size--) {
				write(&compiler->chunk_builder, MACHINE_STORE_VAR);
				write_ulong(&compiler->chunk_builder, reverse_buffer[buffer_size]);
			}
			read_ctok(compiler);
		}
		if (compiler->last_tok.type != tok_open_brace) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}
		read_ctok(compiler);

		int func_returned = 0;
		if (!compile_body(compiler, 1, &func_returned))
			return 0;
		if (!func_returned) {
			write(&compiler->chunk_builder, MACHINE_LOAD_CONST);
			struct value nulbuf;
			init_null_value(&nulbuf);
			write_value(&compiler->chunk_builder, nulbuf);
			free_value(&nulbuf);
			write(&compiler->chunk_builder, MACHINE_RETURN_GOTO);
		}
		write(&compiler->chunk_builder, MACHINE_END_SKIP);
		break;
	}
	case tok_return: {
		if (!func_encapsulated) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}
		read_ctok(compiler);
		compile_expression(compiler, begin, 0);
		write(&compiler->chunk_builder, MACHINE_PROTECT);
		write(&compiler->chunk_builder, MACHINE_RETURN_GOTO);
		*returned = 1;
		break;
	}
	case tok_include: {
		if (func_encapsulated) {
			compiler->last_err = error_unexpected_tok;
			return 0;
		}

		char* file_path = malloc(150);
		if (file_path == NULL) {
			compiler->last_err = error_insufficient_memory;
			return 0;
		}
		read_str(&compiler->scanner, file_path, 0);

		FILE* infile = fopen(file_path, "rb");

		if (infile == NULL) {
			compiler->last_err = error_cannot_open_file;
			return 0;
		}
		unsigned long path_hash = hash(file_path, strlen(file_path));
		for (unsigned char i = 0; i < compiler->imported_files; i++)
			if (compiler->imported_file_hashes[i] == path_hash)
				return 1;
		compiler->imported_file_hashes[compiler->imported_files++] = path_hash;

		fseek(infile, 0, SEEK_END);
		unsigned long fsize = ftell(infile);
		fseek(infile, 0, SEEK_SET);
		char* source = malloc(fsize + 1);
		if (source == NULL) {
			fclose(infile);
			free(file_path);
			compiler->last_err = error_insufficient_memory;
			return 0;
		}
		fread(source, 1, fsize, infile);
		fclose(infile);
		source[fsize] = 0;

		struct compiler temp_compiler;
		init_compiler(&temp_compiler, source);

		if (!compile(&temp_compiler, 0)) {
			compiler->last_err = temp_compiler.last_err;
			return 0;
		}
		struct chunk compiled_chunk = build_chunk(&temp_compiler.chunk_builder);
		write_chunk(&compiler->chunk_builder, compiled_chunk);
		read_ctok(compiler);

		free(source);
		free(file_path);
		break;
	}
	default:
		compiler->last_err = error_unexpected_tok;
		return 0;
	}
	return 1;
}

const int compile_body(struct compiler* compiler, const int func_encapsulated, int* returned) {
	*returned = 0;
	while (compiler->last_tok.type != tok_end && compiler->last_tok.type != tok_close_brace)
	{
		if (!compile_statement(compiler, 1, func_encapsulated, returned))
			return 0;
	}
	read_ctok(compiler);
	return 1;
}

void init_compiler(struct compiler* compiler, const char* source) {
	compiler->imported_files = 0;
	init_scanner(&compiler->scanner, source);
	init_chunk_builder(&compiler->chunk_builder);
	read_ctok(compiler);
}

const int compile(struct compiler* compiler, const int repl_mode) {
	if(!repl_mode)
		write(&compiler->chunk_builder, MACHINE_NEW_FRAME);
	while (compiler->last_tok.type != tok_end)
	{
		if (!compile_statement(compiler, 0, 0, NULL))
			return 0;
	}
	if(!repl_mode)
		write(&compiler->chunk_builder, MACHINE_CLEAN);
	return 1;
}