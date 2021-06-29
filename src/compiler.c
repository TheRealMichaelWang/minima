#define _CRT_SECURE_NO_DEPRECATE
//buffer security is imoortant, but not as much as portability

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hash.h"
#include "error.h"
#include "opcodes.h"
#include "compiler.h"
//#include <cstdio>

#define MATCH_TOK(TOK, TYPE) if(TOK.type != TYPE) { compiler->last_err = ERROR_UNEXPECTED_TOKEN; return 0; }
#define NULL_CHECK(PTR) if (PTR == NULL) { compiler->last_err = ERROR_OUT_OF_MEMORY; return 0; }

#define IS_VALUE_TOK(TOK) (TOK.type < 12 && TOK.type >= 0)
#define IS_STATMENT_TOK(TOK) (TOK.type < 19 && TOK.type >= 9)

#define DECL_VALUE_COMPILER(METHOD_NAME) static const int METHOD_NAME(struct compiler* compiler, const int optimize)
#define DECL_STATMENT_COMPILER(METHOD_NAME) static const int METHOD_NAME(struct compiler* compiler, const uint64_t callee, const int control_encapsulated, const int proc_encapsulated)

#define STD_PROC_CALLEE 0

enum op_precedence {
	PREC_BEGIN,
	PREC_LOGIC,
	PREC_COMP,
	PREC_ADD,
	PREC_MULTIPLY,
	PREC_EXP,
};

enum op_precedence op_precedence[14] = {
	PREC_COMP,
	PREC_COMP,
	PREC_COMP,
	PREC_COMP,
	PREC_COMP,
	PREC_COMP,
	PREC_LOGIC,
	PREC_LOGIC,
	PREC_ADD,
	PREC_ADD,
	PREC_MULTIPLY,
	PREC_MULTIPLY,
	PREC_MULTIPLY,
	PREC_EXP
};

inline static struct token read_ctok(struct compiler* compiler) {
	do
	{
		compiler->last_tok = scanner_read_tok(&compiler->scanner);
	} while (compiler->last_tok.type == TOK_REMARK);
	if (compiler->last_tok.type == TOK_ERROR) {
		compiler->last_err = compiler->scanner.last_err;
	}
	return compiler->last_tok;
}

inline static uint64_t format_label(uint64_t identifier, uint64_t callee, uint64_t arguments) {
	return combine_hash(combine_hash(identifier, arguments), callee);
}

static const int compile_value(struct compiler* compiler, const int expr_optimize);
static const int compile_expression(struct compiler* compiler, enum op_precedence min_prec, const int expr_optimize);

static const int compile_body(struct compiler* compiler, const int func_encapsulated);
static const int compile_statement(struct compiler* compiler, const uint64_t callee, const int control_encapsulated, const int proc_encapsulated);

DECL_VALUE_COMPILER(compile_primative) {
	MATCH_TOK(compiler->last_tok, TOK_PRIMATIVE);
	chunk_write(&compiler->chunk_builder, MACHINE_LOAD_CONST);
	chunk_write_value(&compiler->chunk_builder, compiler->last_tok.payload.primative);
	free_value(&compiler->last_tok.payload.primative);
	read_ctok(compiler);
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
		struct value char_val;
		init_char_value(&char_val, buffer[i]);
		chunk_write_value(&compiler->chunk_builder, char_val);
		free_value(&char_val);
	}
	free(buffer);
	chunk_write(&compiler->chunk_builder, MACHINE_BUILD_COL);
	chunk_write_ulong(&compiler->chunk_builder, len);
	read_ctok(compiler);
	return 1;
}

DECL_VALUE_COMPILER(compile_reference) {
	const int is_ref = (compiler->last_tok.type == TOK_REF);
	if (is_ref)
		read_ctok(compiler);
	MATCH_TOK(compiler->last_tok, TOK_IDENTIFIER);
	chunk_write(&compiler->chunk_builder, MACHINE_LOAD_VAR);
	chunk_write_ulong(&compiler->chunk_builder, compiler->last_tok.payload.identifier);
	read_ctok(compiler);
	while (compiler->last_tok.type == TOK_OPEN_BRACKET || compiler->last_tok.type == TOK_PERIOD)
	{
		if (compiler->last_tok.type == TOK_OPEN_BRACKET) {
			read_ctok(compiler);
			if (!compile_expression(compiler, 0, 1))
				return 0;
			MATCH_TOK(compiler->last_tok, TOK_CLOSE_BRACKET);
			chunk_write(&compiler->chunk_builder, MACHINE_GET_INDEX);
		}
		else {
			MATCH_TOK(read_ctok(compiler), TOK_IDENTIFIER);
			chunk_write(&compiler->chunk_builder, MACHINE_GET_PROPERTY);
			chunk_write_ulong(&compiler->chunk_builder, compiler->last_tok.payload.identifier);
		}
		read_ctok(compiler);
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
		read_ctok(compiler);
		if (!compile_expression(compiler, 0, 0))
			return 0;
		array_size++;
	} while (compiler->last_tok.type == TOK_COMMA);
	MATCH_TOK(compiler->last_tok, TOK_CLOSE_BRACE);

	chunk_write(&compiler->chunk_builder, MACHINE_BUILD_COL);
	chunk_write_ulong(&compiler->chunk_builder, array_size);
	read_ctok(compiler);
	return 1;
}

DECL_VALUE_COMPILER(compile_unary) {
	if (compiler->last_tok.type == TOK_ALLOC) {
		MATCH_TOK(read_ctok(compiler), TOK_OPEN_BRACKET);
		read_ctok(compiler);
		if (!compile_expression(compiler, PREC_BEGIN, 1))
			return 0;
		chunk_write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
		chunk_write(&compiler->chunk_builder, OPERATOR_ALLOC);
		MATCH_TOK(compiler->last_tok, TOK_CLOSE_BRACKET);
		read_ctok(compiler);
	}
	else if (compiler->last_tok.type == TOK_BINARY_OP && compiler->last_tok.payload.bin_op == OPERATOR_SUBTRACT) {
		read_ctok(compiler);
		if (!compile_value(compiler, 1))
			return 0;
		chunk_write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
		chunk_write(&compiler->chunk_builder, OPERATOR_NEGATE);
	}
	else {
		MATCH_TOK(compiler->last_tok, TOK_UNARY_OP);
		enum unary_operator op = compiler->last_tok.payload.uni_op;
		read_ctok(compiler);
		if (!compile_value(compiler, 1))
			return 0;
		chunk_write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
		chunk_write(&compiler->chunk_builder, op);
	}
	return 1;
}

DECL_VALUE_COMPILER(compile_paren) {
	MATCH_TOK(compiler->last_tok, TOK_OPEN_PAREN);
	read_ctok(compiler);
	if (!compile_expression(compiler, PREC_BEGIN, 1))
		return 0;
	MATCH_TOK(compiler->last_tok, TOK_CLOSE_PAREN);
	read_ctok(compiler);
	return 1;
}

DECL_VALUE_COMPILER(compile_goto) {
	int is_proc = compiler->last_tok.type == TOK_GOTO;
	if (!is_proc)
		MATCH_TOK(compiler->last_tok, TOK_EXTERN);
	MATCH_TOK(read_ctok(compiler), TOK_IDENTIFIER);
	uint64_t proc_id = compiler->last_tok.payload.identifier;
	uint64_t arguments = 0;
	int is_record_proc = 0;
	struct chunk_builder temp_builder;
	struct chunk_builder og_builder;
	if (read_ctok(compiler).type == TOK_AS) {
		read_ctok(compiler);
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
			read_ctok(compiler);
			if (!compile_expression(compiler, PREC_BEGIN, 1))
				return 0;
			arguments++;
			if (compiler->last_tok.type != TOK_COMMA)
				break;
		}
		MATCH_TOK(compiler->last_tok, TOK_CLOSE_PAREN);
		read_ctok(compiler);
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
			chunk_write_ulong(&compiler->chunk_builder, format_label(proc_id, 0, arguments));
		}
		chunk_write(&compiler->chunk_builder, MACHINE_CLEAN);
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
	MATCH_TOK(read_ctok(compiler), TOK_IDENTIFIER);
	uint64_t record_id = compiler->last_tok.payload.identifier;
	uint64_t arguments = 1;
	if (read_ctok(compiler).type == TOK_OPEN_PAREN) {
		while (1)
		{
			read_ctok(compiler);
			if (!compile_expression(compiler, PREC_BEGIN, 1))
				return 0;
			arguments++;
			if (compiler->last_tok.type != TOK_COMMA)
				break;
		}
		MATCH_TOK(compiler->last_tok, TOK_CLOSE_PAREN);
		read_ctok(compiler);
	}
	chunk_write(&compiler->chunk_builder, MACHINE_BUILD_RECORD);
	chunk_write_ulong(&compiler->chunk_builder, record_id);
	chunk_write(&compiler->chunk_builder, MACHINE_GOTO_AS);
	chunk_write_ulong(&compiler->chunk_builder, combine_hash(RECORD_INIT_PROC, arguments));
	chunk_write(&compiler->chunk_builder, MACHINE_CLEAN);
	return 1;
}

DECL_VALUE_COMPILER((*value_compilers[12])) = {
	compile_primative,
	compile_string,
	compile_reference,
	compile_reference,
	compile_array,
	compile_new_record,
	compile_paren,
	compile_unary,
	compile_unary,
	compile_unary,
	compile_goto,
	compile_goto
};

static const int compile_value(struct compiler* compiler, const int expr_optimize) {
	if (IS_VALUE_TOK(compiler->last_tok))
		return (*value_compilers[compiler->last_tok.type])(compiler, expr_optimize);
	compiler->last_err = ERROR_UNRECOGNIZED_TOKEN;
	return 0;
}

static const int compile_expression(struct compiler* compiler, enum op_precedence min_prec, const int expr_optimize) {
	char is_id = compiler->last_tok.type == TOK_IDENTIFIER;
	if (!compile_value(compiler, 1)) //lhs
		return 0;
	if (compiler->last_tok.type != TOK_BINARY_OP) {
		if (is_id &&!expr_optimize) {
			chunk_write(&compiler->chunk_builder, MACHINE_EVAL_UNI_OP);
			chunk_write(&compiler->chunk_builder, OPERATOR_COPY);
		}
		return 1;
	}
	while (compiler->last_tok.type == TOK_BINARY_OP && op_precedence[compiler->last_tok.payload.bin_op] > min_prec)
	{
		enum binary_operator op = compiler->last_tok.payload.bin_op;
		read_ctok(compiler);
		if (!compile_expression(compiler, op_precedence[op], 1))
			return 0;
		chunk_write(&compiler->chunk_builder, MACHINE_EVAL_BIN_OP);
		chunk_write(&compiler->chunk_builder, op);
	}
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
	MATCH_TOK(read_ctok(compiler), TOK_IDENTIFIER);
	uint64_t var_id = compiler->last_tok.payload.identifier;
	if (read_ctok(compiler).type == TOK_TO) {
		read_ctok(compiler);
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
				read_ctok(compiler);
				if (!compile_expression(compiler, PREC_BEGIN, 1))
					return 0;
				MATCH_TOK(compiler->last_tok, TOK_CLOSE_BRACKET);
				if (read_ctok(compiler).type == TOK_TO) {
					read_ctok(compiler);
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
				MATCH_TOK(read_ctok(compiler), TOK_IDENTIFIER);
				uint64_t property = compiler->last_tok.payload.identifier;
				if (read_ctok(compiler).type == TOK_TO) {
					read_ctok(compiler);
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

	read_ctok(compiler);
	if (!compile_expression(compiler, PREC_BEGIN, 1))
		return 0;

	chunk_write(&compiler->chunk_builder, MACHINE_COND_SKIP);

	MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
	read_ctok(compiler);

	if (!compile_body(compiler, proc_encapsulated))
		return 0;

	chunk_write(&compiler->chunk_builder, MACHINE_FLAG);
	chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);

	while (compiler->last_tok.type == TOK_ELIF || compiler->last_tok.type == TOK_ELSE)
	{
		if (compiler->last_tok.type == TOK_ELIF) {
			read_ctok(compiler);
			chunk_write(&compiler->chunk_builder, MACHINE_FLAG_SKIP);
			if (!compile_expression(compiler, PREC_BEGIN, 1))
				return 0;
			chunk_write(&compiler->chunk_builder, MACHINE_COND_SKIP);

			MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
			read_ctok(compiler);

			if (!compile_body(compiler, proc_encapsulated))
				return 0;

			chunk_write(&compiler->chunk_builder, MACHINE_FLAG);
			chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);
			chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);
		}
		else {
			read_ctok(compiler);
			chunk_write(&compiler->chunk_builder, MACHINE_FLAG_SKIP);
			MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
			read_ctok(compiler);
			if (!compile_body(compiler, proc_encapsulated))
				return 0;
			chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);
		}
	}
	return 1;
}

DECL_STATMENT_COMPILER(compile_while) {
	MATCH_TOK(compiler->last_tok, TOK_WHILE);
	read_ctok(compiler);

	struct chunk_builder temp_expr_builder;
	init_chunk_builder(&temp_expr_builder);

	struct chunk_builder og_builder = compiler->chunk_builder;
	compiler->chunk_builder = temp_expr_builder;

	if (!compile_expression(compiler, PREC_BEGIN, 1))
		return 0;

	temp_expr_builder = compiler->chunk_builder;
	compiler->chunk_builder = og_builder;

	MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
	read_ctok(compiler);
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

	MATCH_TOK(read_ctok(compiler), TOK_IDENTIFIER);
	uint64_t proc_id = compiler->last_tok.payload.identifier;
	chunk_write(&compiler->chunk_builder, MACHINE_LABEL);
	uint64_t reverse_buffer[50];
	uint32_t buffer_size = 0;
	if (read_ctok(compiler).type == TOK_OPEN_PAREN) {
		while (1)
		{
			MATCH_TOK(read_ctok(compiler), TOK_IDENTIFIER);
			reverse_buffer[buffer_size++] = compiler->last_tok.payload.identifier;
			if (read_ctok(compiler).type != TOK_COMMA) {
				break;
			}
		}
		MATCH_TOK(compiler->last_tok, TOK_CLOSE_PAREN);
		read_ctok(compiler);
	}
	if (callee)
		reverse_buffer[buffer_size++] = RECORD_THIS;
	chunk_write_ulong(&compiler->chunk_builder, format_label(proc_id, callee, buffer_size));
	chunk_write(&compiler->chunk_builder, MACHINE_NEW_FRAME);
	while (buffer_size--) {
		chunk_write(&compiler->chunk_builder, MACHINE_TRACE);
		chunk_write(&compiler->chunk_builder, MACHINE_STORE_VAR);
		chunk_write_ulong(&compiler->chunk_builder, reverse_buffer[buffer_size]);
	}
	MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
	read_ctok(compiler);

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
	chunk_write(&compiler->chunk_builder, MACHINE_RETURN_GOTO);
	chunk_write(&compiler->chunk_builder, MACHINE_END_SKIP);
	return 1;
}

DECL_STATMENT_COMPILER(compile_record) {
	MATCH_TOK(compiler->last_tok, TOK_RECORD);
	MATCH_TOK(read_ctok(compiler), TOK_IDENTIFIER);
	uint64_t record_id = compiler->last_tok.payload.identifier;
	uint64_t base_record_id = 0;
	if (read_ctok(compiler).type == TOK_EXTEND) {
		MATCH_TOK(read_ctok(compiler), TOK_IDENTIFIER);
		base_record_id = compiler->last_tok.payload.identifier;
		read_ctok(compiler);
	}
	MATCH_TOK(compiler->last_tok, TOK_OPEN_BRACE);
	uint64_t property_buffer[500];
	uint64_t properties = 0;
	read_ctok(compiler);
	while (compiler->last_tok.type != TOK_CLOSE_BRACE)
	{
		if (compiler->last_tok.type == TOK_IDENTIFIER) {
			property_buffer[properties++] = compiler->last_tok.payload.identifier;
			read_ctok(compiler);
		}
		else if (compiler->last_tok.type == TOK_PROC) {
			if (!compile_statement(compiler, record_id, 0, 0))
				return 0;
		}
		else {
			compiler->last_err = ERROR_UNEXPECTED_TOKEN;
			return 0;
		}
	}
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
	read_ctok(compiler);
	return 1;
}

DECL_STATMENT_COMPILER(compile_return) {
	MATCH_TOK(compiler->last_tok, TOK_RETURN);
	if (!proc_encapsulated) {
		compiler->last_err = ERROR_UNEXPECTED_TOKEN;
		return 0;
	}
	read_ctok(compiler);
	if (IS_VALUE_TOK(compiler->last_tok)) {
		if (!compile_expression(compiler, PREC_BEGIN, 0))
			return 0;
		chunk_write(&compiler->chunk_builder, MACHINE_TRACE);
	}
	else
		chunk_write_value(&compiler->chunk_builder, const_value_null);
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

	FILE* infile = fopen(file_path, "rb");
	if (!infile) {
		compiler->last_err = ERROR_CANNOT_OPEN_FILE;
		return 0;
	}

	uint64_t path_hash = hash(file_path, strlen(file_path));
	for (uint_fast8_t i = 0; i < compiler->imported_files; i++)
		if (compiler->imported_file_hashes[i] == path_hash)
			return 1;
	compiler->imported_file_hashes[compiler->imported_files++] = path_hash;

	fseek(infile, 0, SEEK_END);
	uint64_t fsize = ftell(infile);
	fseek(infile, 0, SEEK_SET);
	char* source = malloc(fsize + 1);
	NULL_CHECK(source);
	fread(source, 1, fsize, infile);
	fclose(infile);
	source[fsize] = 0;

	struct compiler temp_compiler;
	init_compiler(&temp_compiler, source);

	if (!compile(&temp_compiler, 0)) {
		compiler->last_err = temp_compiler.last_err;
		compiler->scanner = temp_compiler.scanner;
		return 0;
	}
	struct chunk compiled_chunk = build_chunk(&temp_compiler.chunk_builder);
	chunk_write_chunk(&compiler->chunk_builder, compiled_chunk, 1);
	read_ctok(compiler);

	free(source);
	free(file_path);
	return 1;
}

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

static const int compile_statement(struct compiler* compiler, const uint64_t callee, const int control_encapsulated, const int proc_encapsulated) {
	if (IS_STATMENT_TOK(compiler->last_tok))
		return (*statment_compilers[compiler->last_tok.type - TOK_UNARY_OP])(compiler, callee, control_encapsulated, proc_encapsulated);
	compiler->last_err = ERROR_UNRECOGNIZED_TOKEN;
	return 0;
}

static const int compile_body(struct compiler* compiler, const int func_encapsulated) {
	while (compiler->last_tok.type != TOK_END && compiler->last_tok.type != TOK_CLOSE_BRACE)
	{
		if (!compile_statement(compiler, STD_PROC_CALLEE,1, func_encapsulated))
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
		chunk_write(&compiler->chunk_builder, MACHINE_NEW_FRAME);
	while (compiler->last_tok.type != TOK_END)
	{
		if (!compile_statement(compiler, STD_PROC_CALLEE, 0, 0)) {
			return 0;
		}
	}
	if(!repl_mode)
		chunk_write(&compiler->chunk_builder, MACHINE_CLEAN);
	return 1;
}