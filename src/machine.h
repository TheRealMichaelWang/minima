#pragma once

#ifndef MACHINE_H
#define MACHINE_H

#include "value.h"
#include "garbage.h"
#include "chunk.h"
#include "variable.h"
#include "globals.h"
#include "error.h"

//machine codes

enum machine_op_code {
	MACHINE_LOAD_VAR,
    MACHINE_LOAD_CONST,

	MACHINE_STORE_VAR,

	MACHINE_EVAL_BIN_OP,
	MACHINE_EVAL_UNI_OP,

	MACHINE_END_SKIP,

	MACHINE_MARK,

	MACHINE_GOTO,
	MACHINE_GOTO_AS,
	MACHINE_RETURN_GOTO,
	MACHINE_LABEL,

	MACHINE_COND_SKIP,
	MACHINE_COND_RETURN,

	MACHINE_FLAG,
	MACHINE_RESET_FLAG,
	MACHINE_FLAG_SKIP,

	MACHINE_NEW_FRAME,
	MACHINE_CLEAN,

	MACHINE_BUILD_COL,
	MACHINE_BUILD_PROTO,
	MACHINE_BUILD_RECORD,

	MACHINE_GET_INDEX,
	MACHINE_SET_INDEX,
	MACHINE_GET_PROPERTY,
	MACHINE_SET_PROPERTY,

	MACHINE_TRACE,
	MACHINE_POP,

	MACHINE_CALL_EXTERN
};

struct machine {
	unsigned long* position_stack;
	char* position_flags;

	struct value** evaluation_stack;
	char* eval_flags;

	struct var_context* var_stack;

	unsigned long positions;
	unsigned long evals;
	unsigned long call_size;

	char std_flag;
	enum error last_err;

	struct global_cache global_cache;
	struct garbage_collector garbage_collector;
};

void init_machine(struct machine* machine);
void free_machine(struct machine* machine);

void machine_reset_stack(struct machine* machine);

const int machine_execute(struct machine* machine, struct chunk* chunk);

#endif // !MACHINE_H
