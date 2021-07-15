#define _CRT_SECURE_NO_DEPRECATE
//buffer security is imoortant, but not as much as portability

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/runtime/opcodes.h"
#include "include/compiler/chunk.h"
#include "include/compiler/compiler.h"
#include "include/compiler/statements.h"

#define MATCH_TOK(TOK, TYPE) if(TOK.type != TYPE) { compiler->last_err = ERROR_UNEXPECTED_TOKEN; return 0; }
#define NULL_CHECK(PTR) if (PTR == NULL) { compiler->last_err = ERROR_OUT_OF_MEMORY; return 0; }

#define DECL_VALUE_COMPILER(METHOD_NAME) static const int METHOD_NAME(struct compiler* compiler, const int optimize)
#define DECL_STATMENT_COMPILER(METHOD_NAME) static const int METHOD_NAME(struct compiler* compiler, const uint64_t callee, const int control_encapsulated, const int proc_encapsulated)

DECL_VALUE_COMPILER(compile_primative) {
	MATCH_TOK(compiler->last_tok, TOK_PRIMATIVE);
	chunk_write(&compiler->chunk_builder, MACHINE_LOAD_CONST);
	chunk_write_value(&compiler->chunk_builder, compiler->last_tok.payload.primative);
	compiler_read_tok(compiler);
	return 1;
}

DECL_VALUE_COMPILER(compile_string) {
	MATCH_TOK(compiler->last_tok, TOK_STR);
	char* buffer = malloc(150);
	NULL_CHECK(buffer);
	if (!scanner_read_str(&compiler->scanner, buffer, 1)) {
		free(buffer);
		return 0;
	}
	uint64_t len = strlen(buffer);
	for (uint_fast64_t i = 0; i < len; i++) {
		chunk_write(&compiler->chunk_builder, MACHINE_LOAD_CONST);
		chunk_write_value(&compiler->chunk_builder, CHAR_VALUE(buffer[i]));
	}
	free(buffer);
	chunk_write(&compiler->chunk_builder, MACHINE_BUILD_COL);
	chunk_write_ulong(&compiler->chunk_builder, len);
	compiler_read_tok(compiler);
	return 1;
}

DECL_VALUE_COMPILER(compile_reference) {
	const int is_ref = (compiler->last_tok.type == TOK_REF);
	if (is_ref)
		compiler_read_tok(compiler);
	MATCH_TOK(compiler->last_tok, TOK_IDENTIFIER);
	chunk_write(&compiler->chunk_builder, MACHINE_LOAD_VAR);
	chunk_write_ulong(&compiler->chunk_builder, compiler->last_tok.payload.identifier);
	compiler_read_tok(compiler);
	while (compiler->last_tok.type == TOK_OPEN_BRACKET || compiler->last_tok.type == TOK_PERIOD)
	{
		if (compiler->last_tok.type == TOK_OPEN_BRACKET) {
			compiler_read_tok(compiler);
			if (!compile_expression(compiler, 0, 1))
				return 0;
			MATCH_TOK(compiler->last_tok, TOK_CLOSE_BRACKET);
			chunk_write(&compiler->chunk_builder, MACHINE_GET_INDEX);
		}
		else {
			MATCH_TOK(compiler_read_tok(compiler), TOK_IDENTIFIER);
			chunk_write(&compiler->chunk_builder, MACHINE_GET_PROPERTY);
			chunk_write_ulong(&compiler->chunk_builder, compiler->last_tok.payload.identifier);
		}
		compiler_read_tok(compiler);
	}

	if (!is_ref && !optimize) {
		chunk_write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
		chunk_write(&compiler->chunk_builder, OPERATOR_COPY);
	}
	return 1;
}

DECL_VALUE_COMPILER(compile_array) {
	MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
	uint64_t array_size = 0;
	do {
		compiler_read_tok(compiler);
		if (!compile_expression(compiler, 0, 0))
			return 0;
		array_size++;
	} while (compiler->last_tok.type == TOK_COMMA);
	MATCH_TOK(compiler->last_tok, TOK_CLOSE_BRACE);

	chunk_write(&compiler->chunk_builder, MACHINE_BUILD_COL);
	chunk_write_ulong(&compiler->chunk_builder, array_size);
	compiler_read_tok(compiler);
	return 1;
}

DECL_VALUE_COMPILER(compile_unary) {
	if (compiler->last_tok.type == TOK_ALLOC) {
		MATCH_TOK(compiler_read_tok(compiler), TOK_OPEN_BRACKET);
		compiler_read_tok(compiler);
		if (!compile_expression(compiler, PREC_BEGIN, 1))
			return 0;
		chunk_write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
		chunk_write(&compiler->chunk_builder, OPERATOR_ALLOC);
		MATCH_TOK(compiler->last_tok, TOK_CLOSE_BRACKET);
		compiler_read_tok(compiler);
	}
	else if (compiler->last_tok.type == TOK_BINARY_OP && compiler->last_tok.payload.bin_op == OPERATOR_SUBTRACT) {
		compiler_read_tok(compiler);
		if (!compile_value(compiler, 1))
			return 0;
		chunk_write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
		chunk_write(&compiler->chunk_builder, OPERATOR_NEGATE);
	}
	else {
		MATCH_TOK(compiler->last_tok, TOK_UNARY_OP);
		enum unary_operator op = compiler->last_tok.payload.uni_op;
		compiler_read_tok(compiler);
		if (!compile_value(compiler, 1))
			return 0;
		chunk_write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
		chunk_write(&compiler->chunk_builder, op);
	}
	return 1;
}

DECL_VALUE_COMPILER(compile_paren) {
	MATCH_TOK(compiler->last_tok, TOK_OPEN_PAREN);
	compiler_read_tok(compiler);
	if (!compile_expression(compiler, PREC_BEGIN, 1))
		return 0;
	MATCH_TOK(compiler->last_tok, TOK_CLOSE_PAREN);
	compiler_read_tok(compiler);
	return 1;
}

DECL_VALUE_COMPILER(compile_goto) {
	int is_proc = compiler->last_tok.type == TOK_GOTO;
	if (!is_proc)
		MATCH_TOK(compiler->last_tok, TOK_EXTERN);
	MATCH_TOK(compiler_read_tok(compiler), TOK_IDENTIFIER);
	uint64_t proc_id = compiler->last_tok.payload.identifier;
	uint64_t arguments = 0;
	int is_record_proc = 0;
	struct chunk_builder temp_builder;
	struct chunk_builder og_builder;
	if (compiler_read_tok(compiler).type == TOK_AS) {
		compiler_read_tok(compiler);
		init_chunk_builder(&temp_builder);
		og_builder = compiler->chunk_builder;
		compiler->chunk_builder = temp_builder;
		if (!compile_value(compiler, 0))
			return 0;
		temp_builder = compiler->chunk_builder;
		compiler->chunk_builder = og_builder;
		is_record_proc = 1;
		arguments++;
	}
	if (compiler->last_tok.type == TOK_OPEN_PAREN) {
		while (1)
		{
			compiler_read_tok(compiler);
			if (!compile_expression(compiler, PREC_BEGIN, 1))
				return 0;
			arguments++;
			if (compiler->last_tok.type != TOK_COMMA)
				break;
		}
		MATCH_TOK(compiler->last_tok, TOK_CLOSE_PAREN);
		compiler_read_tok(compiler);
	}
	if (is_proc) {
		if (is_record_proc) {
			struct chunk temp_chunk = build_chunk(&temp_builder);
			chunk_write_chunk(&compiler->chunk_builder, temp_chunk, 1);
			chunk_write(&compiler->chunk_builder, MACHINE_GOTO_AS);
			chunk_write_ulong(&compiler->chunk_builder, combine_hash(proc_id, arguments));
		}
		else {
			chunk_write(&compiler->chunk_builder, MACHINE_GOTO);
			chunk_write_ulong(&compiler->chunk_builder, combine_hash(combine_hash(proc_id, arguments), 0));
		}
	}
	else {
		chunk_write(&compiler->chunk_builder, MACHINE_CALL_EXTERN);
		chunk_write_ulong(&compiler->chunk_builder, proc_id);
		chunk_write_ulong(&compiler->chunk_builder, arguments);
	}
	return 1;
}

DECL_VALUE_COMPILER(compile_new_record) {
	MATCH_TOK(compiler->last_tok, TOK_NEW);
	MATCH_TOK(compiler_read_tok(compiler), TOK_IDENTIFIER);
	uint64_t record_id = compiler->last_tok.payload.identifier;
	uint64_t arguments = 1;
	if (compiler_read_tok(compiler).type == TOK_OPEN_PAREN) {
		while (1)
		{
			compiler_read_tok(compiler);
			if (!compile_expression(compiler, PREC_BEGIN, 1))
				return 0;
			arguments++;
			if (compiler->last_tok.type != TOK_COMMA)
				break;
		}
		MATCH_TOK(compiler->last_tok, TOK_CLOSE_PAREN);
		compiler_read_tok(compiler);
	}
	chunk_write(&compiler->chunk_builder, MACHINE_BUILD_RECORD);
	chunk_write_ulong(&compiler->chunk_builder, record_id);
	chunk_write(&compiler->chunk_builder, MACHINE_GOTO_AS);
	chunk_write_ulong(&compiler->chunk_builder, combine_hash(RECORD_INIT_PROC, arguments));
	return 1;
}

DECL_VALUE_COMPILER(compile_hashtag) {
	MATCH_TOK(compiler->last_tok, TOK_HASHTAG);
	MATCH_TOK(compiler_read_tok(compiler), TOK_IDENTIFIER);
	chunk_write(&compiler->chunk_builder, MACHINE_LOAD_CONST);
	chunk_write_value(&compiler->chunk_builder, ID_VALUE(compiler->last_tok.payload.identifier));
	compiler_read_tok(compiler);
	return 1;
}

DECL_STATMENT_COMPILER(compile_value_statement) {
	if (!compile_value(compiler, 0))
		return 0;
	chunk_write(&compiler->chunk_builder, MACHINE_POP);
	return 1;
}

DECL_STATMENT_COMPILER(compile_set) {
	MATCH_TOK(compiler->last_tok, TOK_SET);
	MATCH_TOK(compiler_read_tok(compiler), TOK_IDENTIFIER);
	uint64_t var_id = compiler->last_tok.payload.identifier;
	if (compiler_read_tok(compiler).type == TOK_TO) {
		compiler_read_tok(compiler);
		if (!compile_expression(compiler, PREC_BEGIN, 0))
			return 0;
		chunk_write(&compiler->chunk_builder, MACHINE_STORE_VAR);
		chunk_write_ulong(&compiler->chunk_builder, var_id);
	}
	else {
		chunk_write(&compiler->chunk_builder, MACHINE_LOAD_VAR);
		chunk_write_ulong(&compiler->chunk_builder, var_id);
		while (1)
		{
			if (compiler->last_tok.type == TOK_OPEN_BRACKET) {
				compiler_read_tok(compiler);
				if (!compile_expression(compiler, PREC_BEGIN, 1))
					return 0;
				MATCH_TOK(compiler->last_tok, TOK_CLOSE_BRACKET);
				if (compiler_read_tok(compiler).type == TOK_TO) {
					compiler_read_tok(compiler);
					if (!compile_expression(compiler, PREC_BEGIN, 0))
						return 0;
					chunk_write(&compiler->chunk_builder, MACHINE_SET_INDEX);
					break;
				}
				else {
					chunk_write(&compiler->chunk_builder, MACHINE_GET_INDEX);
				}
			}
			else if (compiler->last_tok.type == TOK_PERIOD) {
				MATCH_TOK(compiler_read_tok(compiler), TOK_IDENTIFIER);
				uint64_t property = compiler->last_tok.payload.identifier;
				if (compiler_read_tok(compiler).type == TOK_TO) {
					compiler_read_tok(compiler);
					if (!compile_expression(compiler, PREC_BEGIN, 0))
						return 0;
					chunk_write(&compiler->chunk_builder, MACHINE_SET_PROPERTY);
					chunk_write_ulong(&compiler->chunk_builder, property);
					break;
				}
				else {
					chunk_write(&compiler->chunk_builder, MACHINE_GET_PROPERTY);
					chunk_write_ulong(&compiler->chunk_builder, property);
				}
			}
			else {
				compiler->last_err = ERROR_UNEXPECTED_TOKEN;
				return 0;
			}
		}
	}
	return 1;
}

DECL_STATMENT_COMPILER(compile_if) {
	MATCH_TOK(compiler->last_tok, TOK_IF);
	chunk_write(&compiler->chunk_builder, MACHINE_RESET_FLAG);

	compiler_read_tok(compiler);
	if (!compile_expression(compiler, PREC_BEGIN, 1))
		return 0;

	chunk_write(&compiler->chunk_builder, MACHINE_COND_SKIP);

	MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
	compiler_read_tok(compiler);

	if (!compile_body(compiler, proc_encapsulated))
		return 0;

	chunk_write(&compiler->chunk_builder, MACHINE_FLAG);
	chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);

	while (compiler->last_tok.type == TOK_ELIF || compiler->last_tok.type == TOK_ELSE)
	{
		if (compiler->last_tok.type == TOK_ELIF) {
			compiler_read_tok(compiler);
			chunk_write(&compiler->chunk_builder, MACHINE_FLAG_SKIP);
			if (!compile_expression(compiler, PREC_BEGIN, 1))
				return 0;
			chunk_write(&compiler->chunk_builder, MACHINE_COND_SKIP);

			MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
			compiler_read_tok(compiler);

			if (!compile_body(compiler, proc_encapsulated))
				return 0;

			chunk_write(&compiler->chunk_builder, MACHINE_FLAG);
			chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);
			chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);
		}
		else {
			compiler_read_tok(compiler);
			chunk_write(&compiler->chunk_builder, MACHINE_FLAG_SKIP);
			MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
			compiler_read_tok(compiler);
			if (!compile_body(compiler, proc_encapsulated))
				return 0;
			chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);
		}
	}
	return 1;
}

DECL_STATMENT_COMPILER(compile_while) {
	MATCH_TOK(compiler->last_tok, TOK_WHILE);
	compiler_read_tok(compiler);

	struct chunk_builder temp_expr_builder;
	init_chunk_builder(&temp_expr_builder);

	struct chunk_builder og_builder = compiler->chunk_builder;
	compiler->chunk_builder = temp_expr_builder;

	if (!compile_expression(compiler, PREC_BEGIN, 1))
		return 0;

	temp_expr_builder = compiler->chunk_builder;
	compiler->chunk_builder = og_builder;

	MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
	compiler_read_tok(compiler);
	struct chunk temp_expr_chunk = build_chunk(&temp_expr_builder);

	chunk_write_chunk(&compiler->chunk_builder, temp_expr_chunk, 0);
	chunk_write(&compiler->chunk_builder, MACHINE_COND_SKIP);

	chunk_write(&compiler->chunk_builder, MACHINE_MARK);

	if (!compile_body(compiler, proc_encapsulated))
		return 0;

	chunk_write_chunk(&compiler->chunk_builder, temp_expr_chunk, 1);

	chunk_write(&compiler->chunk_builder, MACHINE_COND_RETURN);
	chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);
	return 1;
}

DECL_STATMENT_COMPILER(compile_proc) {
	MATCH_TOK(compiler->last_tok, TOK_PROC);
	if (control_encapsulated) {
		compiler->last_err = ERROR_UNEXPECTED_TOKEN;
		return 0;
	}

	uint64_t proc_id;
	compiler_read_tok(compiler);
	if (callee && compiler->last_tok.type == TOK_BINARY_OP)
		proc_id = combine_hash((uint64_t)compiler->last_tok.payload.bin_op, BINARY_OVERLOAD);
	else if (callee && compiler->last_tok.type == TOK_UNARY_OP)
		proc_id = combine_hash((uint64_t)compiler->last_tok.payload.bin_op, UNARY_OVERLOAD);
	else {
		MATCH_TOK(compiler->last_tok, TOK_IDENTIFIER);
		proc_id = compiler->last_tok.payload.identifier;
	}

	chunk_write(&compiler->chunk_builder, MACHINE_LABEL);
	uint64_t reverse_buffer[50];
	uint32_t buffer_size = 0;
	if (compiler_read_tok(compiler).type == TOK_OPEN_PAREN) {
		while (1)
		{
			MATCH_TOK(compiler_read_tok(compiler), TOK_IDENTIFIER);
			reverse_buffer[buffer_size++] = compiler->last_tok.payload.identifier;
			if (compiler_read_tok(compiler).type != TOK_COMMA) {
				break;
			}
		}
		MATCH_TOK(compiler->last_tok, TOK_CLOSE_PAREN);
		compiler_read_tok(compiler);
	}
	if (callee) {
		reverse_buffer[buffer_size++] = RECORD_THIS;
		if (proc_id == 5862393 && buffer_size == 1)
			proc_id = combine_hash((uint64_t)OPERATOR_NEGATE, UNARY_OVERLOAD);
	}
	
	chunk_write_ulong(&compiler->chunk_builder, combine_hash(combine_hash(proc_id, buffer_size), callee));
	chunk_write(&compiler->chunk_builder, MACHINE_NEW_FRAME);
	while (buffer_size--) {
		chunk_write(&compiler->chunk_builder, MACHINE_TRACE);
		chunk_write(&compiler->chunk_builder, MACHINE_STORE_VAR);
		chunk_write_ulong(&compiler->chunk_builder, reverse_buffer[buffer_size]);
	}
	MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
	compiler_read_tok(compiler);

	if (!compile_body(compiler, 1))
		return 0;
	if (callee && proc_id == RECORD_INIT_PROC) {
		chunk_write(&compiler->chunk_builder, MACHINE_LOAD_VAR);
		chunk_write_ulong(&compiler->chunk_builder, RECORD_THIS);
		chunk_write(&compiler->chunk_builder, MACHINE_TRACE);
	}
	else {
		chunk_write(&compiler->chunk_builder, MACHINE_LOAD_CONST);
		chunk_write_value(&compiler->chunk_builder, const_value_null);
	}
	chunk_write(&compiler->chunk_builder, MACHINE_CLEAN);
	chunk_write(&compiler->chunk_builder, MACHINE_RETURN_GOTO);
	chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);
	return 1;
}

DECL_STATMENT_COMPILER(compile_record) {
	MATCH_TOK(compiler->last_tok, TOK_RECORD);
	MATCH_TOK(compiler_read_tok(compiler), TOK_IDENTIFIER);
	uint64_t record_id = compiler->last_tok.payload.identifier;
	uint64_t base_record_id = 0;
	if (compiler_read_tok(compiler).type == TOK_EXTEND) {
		MATCH_TOK(compiler_read_tok(compiler), TOK_IDENTIFIER);
		base_record_id = compiler->last_tok.payload.identifier;
		compiler_read_tok(compiler);
	}

	uint64_t property_buffer[500];
	uint64_t properties = 0;
	
	if (compiler->last_tok.type == TOK_OPEN_BRACE) {
		compiler_read_tok(compiler);
		while (compiler->last_tok.type != TOK_CLOSE_BRACE)
		{
			if (compiler->last_tok.type == TOK_IDENTIFIER) {
				property_buffer[properties++] = compiler->last_tok.payload.identifier;
				compiler_read_tok(compiler);
			}
			else if (compiler->last_tok.type == TOK_PROC) {
				if (!compile_proc(compiler, record_id, 0, 1))
					return 0;
			}
			else {
				compiler->last_err = ERROR_UNEXPECTED_TOKEN;
				return 0;
			}
		}
		compiler_read_tok(compiler);
	}
	else if (!base_record_id)
		MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);

	chunk_write(&compiler->chunk_builder, MACHINE_BUILD_PROTO);
	chunk_write_ulong(&compiler->chunk_builder, record_id);
	chunk_write_ulong(&compiler->chunk_builder, properties);
	while (properties--)
		chunk_write_ulong(&compiler->chunk_builder, property_buffer[properties]);
	if (base_record_id) {
		chunk_write(&compiler->chunk_builder, MACHINE_INHERIT_REC);
		chunk_write_ulong(&compiler->chunk_builder, record_id);
		chunk_write_ulong(&compiler->chunk_builder, base_record_id);
	}
	return 1;
}

DECL_STATMENT_COMPILER(compile_return) {
	MATCH_TOK(compiler->last_tok, TOK_RETURN);
	if (!proc_encapsulated) {
		compiler->last_err = ERROR_UNEXPECTED_TOKEN;
		return 0;
	}
	compiler_read_tok(compiler);
	if (IS_VALUE_TOK(compiler->last_tok)) {
		if (!compile_expression(compiler, PREC_BEGIN, 0))
			return 0;
		chunk_write(&compiler->chunk_builder, MACHINE_TRACE);
	}
	else {
		chunk_write(&compiler->chunk_builder, MACHINE_LOAD_CONST);
		chunk_write_value(&compiler->chunk_builder, const_value_null);
	}
	chunk_write(&compiler->chunk_builder, MACHINE_CLEAN);
	chunk_write(&compiler->chunk_builder, MACHINE_RETURN_GOTO);
	return 1;
}

DECL_STATMENT_COMPILER(compile_include) {
	MATCH_TOK(compiler->last_tok, TOK_INCLUDE);
	if (proc_encapsulated) {
		compiler->last_err = ERROR_UNEXPECTED_TOKEN;
		return 0;
	}

	char* file_path = malloc(150);
	NULL_CHECK(file_path);
	scanner_read_str(&compiler->scanner, file_path, 0);

	uint64_t path_hash = hash(file_path, strlen(file_path));
	for (uint_fast8_t i = 0; i < compiler->imported_files; i++)
		if (compiler->imported_file_hashes[i] == path_hash) {
			free(file_path);
			compiler_read_tok(compiler);
			return 1;
		}
	compiler->imported_file_hashes[compiler->imported_files++] = path_hash;

	FILE* infile = fopen(file_path, "rb");
	if (!infile) {
		free(file_path);
		compiler->last_err = ERROR_CANNOT_OPEN_FILE;
		return 0;
	}
	free(file_path);

	fseek(infile, 0, SEEK_END);
	uint64_t fsize = ftell(infile);
	fseek(infile, 0, SEEK_SET);
	char* source = malloc((fsize + 1) * sizeof(char));
	NULL_CHECK(source);
	fread(source, sizeof(char), fsize, infile);
	fclose(infile);
	source[fsize] = 0;

	struct scanner my_scanner = compiler->scanner;
	init_scanner(&compiler->scanner, source);
	
	compiler_read_tok(compiler);
	if (!compile(compiler, 0)) 
		return 0;
	free(source);

	compiler->scanner = my_scanner;
	compiler_read_tok(compiler);

	return 1;
}

DECL_VALUE_COMPILER((*value_compilers[13])) = {
	compile_primative,
	compile_string,
	compile_reference,
	compile_reference,
	compile_array,
	compile_new_record,
	compile_paren,
	compile_unary,
	compile_unary,
	compile_hashtag,
	compile_unary,
	compile_goto,
	compile_goto
};

DECL_STATMENT_COMPILER((*statment_compilers[10])) = {
	compile_value_statement,
	compile_value_statement,
	compile_value_statement,
	compile_set,
	compile_if,
	compile_while,
	compile_proc,
	compile_record,
	compile_return,
	compile_include
};

const int compile_value(struct compiler* compiler, const int expr_optimize) {
	if (IS_VALUE_TOK(compiler->last_tok))
		return (*value_compilers[compiler->last_tok.type])(compiler, expr_optimize);
	compiler->last_err = ERROR_UNRECOGNIZED_TOKEN;
	return 0;
}

const int compile_statement(struct compiler* compiler, const uint64_t callee, const int control_encapsulated, const int proc_encapsulated) {
	if (IS_STATMENT_TOK(compiler->last_tok))
		return (*statment_compilers[compiler->last_tok.type - TOK_UNARY_OP])(compiler, callee, control_encapsulated, proc_encapsulated);
	compiler->last_err = ERROR_UNRECOGNIZED_TOKEN;
	return 0;
}