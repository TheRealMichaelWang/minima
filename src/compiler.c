#include <stdlib.h>
#include <string.h>
#include "include/opcodes.h"
#include "include/tokens.h"
#include "include/globals.h"
#include "include/statements.h"
#include "include/compiler.h"

static const int compiler_link_chunk(struct compiler* compiler, struct chunk* chunk, uint64_t offset) {
	uint64_t** skip_stack = malloc(64);
	ERROR_ALLOC_CHECK(skip_stack);

	uint8_t skips = 0;

	while (chunk->last_code != MACHINE_END)
	{
		enum op_code op = chunk->last_code;
		switch (op)
		{
		case MACHINE_LABEL:
		case MACHINE_COND_SKIP:
		case MACHINE_FLAG_SKIP: {
			skip_stack[skips++] = chunk_read_size(chunk, sizeof(uint64_t));
			if (op == MACHINE_LABEL) {
				uint64_t label_id = chunk_read_ulong(chunk);
				if (!cache_insert_label(&compiler->machine->global_cache, label_id, chunk->pos + offset)) {
					compiler->last_err = ERROR_LABEL_REDEFINE;
					return 0;
				}
			}
			break;
		}
		case MACHINE_END_SKIP: {
			*skip_stack[--skips] = chunk->pos - 1 + offset;
			break;
		}
		case MACHINE_CALL_EXTERN:
		case MACHINE_INHERIT_REC:
			chunk_read_size(chunk, sizeof(uint64_t));
		case MACHINE_GOTO:
		case MACHINE_GOTO_AS:
		case MACHINE_STORE_VAR:
		case MACHINE_LOAD_VAR:
		case MACHINE_BUILD_COL:
		case MACHINE_SET_PROPERTY:
		case MACHINE_GET_PROPERTY:
		case MACHINE_BUILD_RECORD:
		case MACHINE_TRACE:
			chunk_read_size(chunk, sizeof(uint64_t));
			break;
		case MACHINE_LOAD_CONST:
			chunk_read_size(chunk, sizeof(struct value));
			break;
		case MACHINE_EVAL_BIN_OP:
		case MACHINE_EVAL_UNI_OP:
			chunk_read_size(chunk, sizeof(uint8_t));
			break;
		case MACHINE_BUILD_PROTO: {
			chunk_read_size(chunk, sizeof(uint64_t));
			uint64_t i = chunk_read_ulong(chunk);
			while (i--)
				chunk_read_size(chunk, sizeof(uint64_t));
			break;
		}
		}
		chunk_read_opcode(chunk);
	}
	free(skip_stack);
	chunk->pos = 0;
	chunk_read_opcode(chunk);
	if (skips)
		return 0;
	while (chunk->last_code != MACHINE_END)
	{
		enum op_code op = chunk->last_code;
		switch (op)
		{
		case MACHINE_LABEL:
			chunk_read_size(chunk, sizeof(uint64_t));
		case MACHINE_COND_SKIP:
		case MACHINE_FLAG_SKIP: {
			chunk_read_size(chunk, sizeof(uint64_t));
			break;
		}
		case MACHINE_CALL_EXTERN:
		case MACHINE_INHERIT_REC:
			chunk_read_size(chunk, sizeof(uint64_t));
		case MACHINE_GOTO_AS:
		case MACHINE_STORE_VAR:
		case MACHINE_LOAD_VAR:
		case MACHINE_BUILD_COL:
		case MACHINE_SET_PROPERTY:
		case MACHINE_GET_PROPERTY:
		case MACHINE_BUILD_RECORD:
		case MACHINE_TRACE:
			chunk_read_size(chunk, sizeof(uint64_t));
			break;
		case MACHINE_GOTO: {
			uint64_t* pos_slot = chunk_read_size(chunk, sizeof(uint64_t));
			uint64_t pos = cache_retrieve_pos(&compiler->machine->global_cache, *pos_slot);
			if (!pos) {
				compiler->last_err = ERROR_LABEL_UNDEFINED;
				return 0;
			}
			*pos_slot = pos;
			break;
		}
		case MACHINE_LOAD_CONST:
			chunk_read_size(chunk, sizeof(struct value));
			break;
		case MACHINE_EVAL_BIN_OP:
		case MACHINE_EVAL_UNI_OP:
			chunk_read_size(chunk, sizeof(uint8_t));
			break;
		case MACHINE_BUILD_PROTO: {
			chunk_read_size(chunk, sizeof(uint64_t));
			uint64_t i = chunk_read_ulong(chunk);
			while (i--)
				chunk_read_size(chunk, sizeof(uint64_t));
			break;
		}
		}
		chunk_read_opcode(chunk);
	}
	chunk->pos = 0;
	chunk_read_opcode(chunk);
	return 1;
}

void init_compiler(struct compiler* compiler, struct machine* machine, const char* include_dir, const char* source, const char* file) {
	compiler->imported_files = 0;
	compiler->include_dir = include_dir;
	compiler->machine = machine;
	compiler->include_dir_len = strlen(include_dir);
	init_scanner(&compiler->scanner, source, file);
	init_chunk_builder(&compiler->code_builder);
	init_chunk_builder(&compiler->data_builder);
	compiler_read_tok(compiler);
}

const int compile(struct compiler* compiler, struct loc_table* loc_table, const int repl_mode, const int link_output, uint64_t prev_offset) {
	if(!repl_mode)
		chunk_write_opcode(&compiler->code_builder, MACHINE_NEW_FRAME);
	
	while (compiler->last_tok.type != TOK_END)
		if (!compile_statement(compiler, &compiler->code_builder, loc_table, 0, 0, 0))
			return 0;

	if(!repl_mode)
		chunk_write_opcode(&compiler->code_builder, MACHINE_CLEAN);

	if (link_output) {
		struct chunk_builder sum;
		init_chunk_builder(&sum);

		if (!chunk_write_chunk(&sum, build_chunk(&compiler->data_builder), 1) ||
			!chunk_write_chunk(&sum, build_chunk(&compiler->code_builder), 1)) {
			compiler->last_err = ERROR_OUT_OF_MEMORY;
			return 0;
		}

		compiler->result = build_chunk(&sum);
		ERROR_ALLOC_CHECK(compiler_link_chunk(compiler, &compiler->result, prev_offset));
	}

	compiler->last_err = ERROR_SUCCESS;
	return 1;
}

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

const int compile_body(struct compiler* compiler, struct chunk_builder* builder, struct loc_table* loc_table, uint64_t callee, uint64_t proc_encapsulated) {
	if (compiler->last_tok.type != TOK_OPEN_BRACE) {
		compiler->last_err = ERROR_UNEXPECTED_TOKEN;
		return 0;
	}

	compiler_read_tok(compiler);

	while (compiler->last_tok.type != TOK_END && compiler->last_tok.type != TOK_CLOSE_BRACE)
	{
		if (!compile_statement(compiler, builder, loc_table, callee, 1, proc_encapsulated))
			return 0;
	}
	compiler_read_tok(compiler);
	return 1;
}