#pragma once

#ifndef MACHINE_H
#define MACHINE_H

#include <stdint.h>
#include "value.h"
#include "garbage.h"
#include "variable.h"
#include "globals.h"
#include "../compiler/chunk.h"

#define MACHINE_MAX_POSITIONS 20000
#define MACHINE_MAX_EVALS 10000
#define MACHINE_MAX_CALLS 10000

struct machine {
	uint64_t* position_stack;
	char* position_flags;

	struct value** evaluation_stack;
	char* eval_flags;

	struct var_context* var_stack;

	uint64_t positions;
	uint64_t evals;
	uint64_t call_size;

	char std_flag;
	enum error last_err;

	struct global_cache global_cache;
	struct garbage_collector garbage_collector;
};

const int init_machine(struct machine* machine);
void free_machine(struct machine* machine);

void machine_reset(struct machine* machine);

const int machine_execute(struct machine* machine, struct chunk* chunk);

#endif // !MACHINE_H
