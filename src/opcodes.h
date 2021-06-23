#pragma once

#ifndef OPCODE_H
#define OPCODE_H

#include "machine.h"

//machine op-codes
enum op_code {
	MACHINE_LOAD_CONST,
	MACHINE_LOAD_VAR,

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
	MACHINE_TRACE,
	MACHINE_POP,

	MACHINE_BUILD_COL,
	MACHINE_BUILD_PROTO,
	MACHINE_BUILD_RECORD,

	MACHINE_GET_INDEX,
	MACHINE_SET_INDEX,
	MACHINE_GET_PROPERTY,
	MACHINE_SET_PROPERTY,

	MACHINE_CALL_EXTERN,
	MACHINE_END //last one isn't really an opcode, just an end signal
};

const int handle_opcode(enum op_code op, struct machine* machine, struct chunk* chunk);

#endif // !OPCODE_H