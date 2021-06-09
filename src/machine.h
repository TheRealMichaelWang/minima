#pragma once

#ifndef MACHINE_H
#define MACHINE_H

#include "value.h"
#include "garbage.h"
#include "chunk.h"
#include "variable.h"
#include "labels.h"
#include "builtins.h"

//machine codes

#define MACHINE_LOAD_VAR 1
#define MACHINE_LOAD_CONST 2

#define MACHINE_STORE_VAR 3

#define MACHINE_EVAL_BIN_OP 4
#define MACHINE_EVAL_UNI_OP 5

#define MACHINE_SKIP 6
#define MACHINE_END_SKIP 7

#define MACHINE_MARK 8

#define MACHINE_GOTO 9
#define MACHINE_RETURN_GOTO 10
#define MACHINE_LABEL 11

#define MACHINE_COND_SKIP 12
#define MACHINE_COND_RETURN 13

#define MACHINE_FLAG 14
#define MACHINE_RESET_FLAG 15
#define MACHINE_FLAG_SKIP 16

#define MACHINE_NEW_FRAME 17
#define MACHINE_CLEAN 18

#define MACHINE_BUILD_COL 19

#define MACHINE_GET_INDEX 20
#define MACHINE_SET_INDEX 21

#define MACHINE_PROTECT 22
#define MACHINE_POP 23

#define MACHINE_CALL_EXTERN 24

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
	char last_err;

	struct garbage_collector garbage_collector;
	struct label_cache label_cache;
	struct builtin_register builtin_register;
};

void init_machine(struct machine* machine);
void free_machine(struct machine* machine);

void reset_stack(struct machine* machine);

int execute(struct machine* machine, struct chunk* chunk);

#endif // !MACHINE_H
