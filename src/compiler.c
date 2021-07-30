#include <stdlib.h>
#include <string.h>
#include "include/opcodes.h"
#include "include/tokens.h"
#include "include/statements.h"
#include "include/compiler.h"

struct token compiler_read_tok(struct compiler* compiler) {
	do {
		compiler->last_tok = scanner_read_tok(&compiler->scanner);
	} while (compiler->last_tok.type == TOK_REMARK);
	if (compiler->last_tok.type == TOK_ERROR) {
		compiler->last_err = compiler->scanner.last_err;
	}
	return compiler->last_tok;
}

const int compile_expression(struct compiler* compiler, struct chunk_builder* builder, enum op_precedence min_prec, const int optimize_copy, uint64_t optimize_goto) {
	char is_id = compiler->last_tok.type == TOK_IDENTIFIER;
	if (!compile_value(compiler, builder, !optimize_goto, optimize_goto)) //lhs
		return 0;
	if (compiler->last_tok.type != TOK_BINARY_OP) {
		if (is_id && !optimize_copy && !optimize_goto) {
			chunk_write_opcode(builder, MACHINE_EVAL_UNI_OP);
			chunk_write_uni_op(builder, OPERATOR_COPY);
		}
		return 1;
	}
	while (compiler->last_tok.type == TOK_BINARY_OP && op_precedence[compiler->last_tok.payload.bin_op] > min_prec)
	{
		enum binary_operator op = compiler->last_tok.payload.bin_op;
		compiler_read_tok(compiler);
		if (!compile_expression(compiler, builder, op_precedence[op], 1, optimize_goto))
			return 0;
		chunk_write_opcode(builder, MACHINE_EVAL_BIN_OP);
		chunk_write_bin_op(builder, op);
	}
	return 1;
}

const int compile_body(struct compiler* compiler, struct chunk_builder* builder, uint64_t callee, uint64_t proc_encapsulated) {
	if (compiler->last_tok.type != TOK_OPEN_BRACE) {
		compiler->last_err = ERROR_UNEXPECTED_TOKEN;
		return 0;
	}

	compiler_read_tok(compiler);

	while (compiler->last_tok.type != TOK_END && compiler->last_tok.type != TOK_CLOSE_BRACE)
	{
		if (!compile_statement(compiler, builder, callee, 1, proc_encapsulated))
			return 0;
	}
	compiler_read_tok(compiler);
	return 1;
}

void init_compiler(struct compiler* compiler, const char* include_dir, const char* source) {
	compiler->imported_files = 0;
	compiler->include_dir = include_dir;
	compiler->include_dir_len = strlen(include_dir);
	init_scanner(&compiler->scanner, source);
	init_chunk_builder(&compiler->code_builder);
	init_chunk_builder(&compiler->data_builder);
	compiler_read_tok(compiler);
}

const int compile(struct compiler* compiler, const int repl_mode) {
	if(!repl_mode)
		chunk_write_opcode(&compiler->code_builder, MACHINE_NEW_FRAME);
	
	while (compiler->last_tok.type != TOK_END)
	{
		if (!compile_statement(compiler, &compiler->code_builder, 0, 0, 0)) {
			return 0;
		}
	}

	if(!repl_mode)
		chunk_write_opcode(&compiler->code_builder, MACHINE_CLEAN);
	return 1;
}

struct chunk compiler_get_chunk(struct compiler* compiler, uint64_t prev_offset) {
	struct chunk_builder sum;
	init_chunk_builder(&sum);

	chunk_write_chunk(&sum, build_chunk(&compiler->data_builder), 1);
	chunk_write_chunk(&sum, build_chunk(&compiler->code_builder), 1);
	
	struct chunk build = build_chunk(&sum);
	chunk_optimize(&build, prev_offset);

	return build;
}