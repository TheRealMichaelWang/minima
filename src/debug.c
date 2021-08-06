#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "include/opcodes.h"
#include "include/io.h"
#include "include/error.h"
#include "include/debug.h"

const int init_loc_table(struct loc_table* loc_table, char* initial_file) {
	ERROR_ALLOC_CHECK(loc_table->locations = malloc((loc_table->alloced_locs = 64) * sizeof(struct loc_info)));
	ERROR_ALLOC_CHECK(loc_table->to_free = malloc((loc_table->alloced_files = 64) * sizeof(char*)));
	ERROR_ALLOC_CHECK(loc_table->file_stack = malloc(64 * sizeof(const char*)));
	loc_table->files = 0;
	loc_table->current_file = 0;
	loc_table->loc_entries = 0;
	loc_table->global_offset = 0;
	loc_table->finilazed_flag = 0;
	loc_table->file_stack[0] = initial_file;
	return 1;
}

void free_loc_table(struct loc_table* loc_table) {
	for (uint_fast64_t i = 0; i < loc_table->files; i++)
		free(loc_table->to_free[i]);
	free(loc_table->to_free);
	free(loc_table->locations);
	free(loc_table->file_stack);
}

const int loc_table_include(struct loc_table* loc_table, char* file) {
	if (loc_table->files == loc_table->alloced_files) {
		char** new_free = realloc(loc_table->to_free, (loc_table->alloced_files *= 2) * sizeof(char*));
		if (!new_free)
			return 0;
		loc_table->to_free = new_free;
	}
	loc_table->to_free[loc_table->files++] = file;
	loc_table->file_stack[++loc_table->current_file] = file;
	return 1;
}

void loc_table_uninclude(struct loc_table* loc_table) {
	loc_table->current_file--;
}

const int loc_table_insert(struct loc_table* loc_table, struct compiler* compiler, struct chunk_builder* builder) {
	loc_table->finilazed_flag = 0;
	if (loc_table->loc_entries == loc_table->alloced_locs) {
		struct loc_info** new_entries = realloc(loc_table->locations, (loc_table->alloced_locs *= 2) * sizeof(struct loc_info));
		if (!new_entries)
			return 0;
		loc_table->locations = new_entries;
	}
	loc_table->locations[loc_table->loc_entries++] = (struct loc_info){
		.file = loc_table->file_stack[loc_table->current_file],
		.chunk_loc = builder->size + loc_table->global_offset,
		.src_col = compiler->scanner.col,
		.src_row = compiler->scanner.row,
		.offset_flag = (builder == &compiler->code_builder),
		.keep_flag = 0
	};
	return 1;
}

void loc_table_finalize(struct loc_table* loc_table, struct compiler* compiler, int keep) {
	loc_table->finilazed_flag = 1;
	for (uint_fast64_t i = 0; i < loc_table->loc_entries; i++) {
		if (loc_table->locations[i].offset_flag) {
			loc_table->locations[i].chunk_loc += compiler->data_builder.size;
			loc_table->locations[i].offset_flag = 0;
		}
		if (keep)
			loc_table->locations[i].keep_flag = 1;
	}
	int sorted = 0;
	do {
		sorted = 1;
		for(uint_fast64_t i = 1; i < loc_table->loc_entries; i++)
			if (loc_table->locations[i - 1].chunk_loc > loc_table->locations[i].chunk_loc) {
				struct loc_info temp = loc_table->locations[i];
				loc_table->locations[i] = loc_table->locations[i - 1];
				loc_table->locations[i - 1] = temp;
				sorted = 0;
			}
	} while (!sorted);
}

void loc_table_dispose(struct loc_table* loc_table) {
	if (loc_table->finilazed_flag) {
		while (!loc_table->locations[loc_table->loc_entries - 1].keep_flag)
			loc_table->loc_entries--;
	}
	else {
		uint64_t upper_bound = loc_table->loc_entries;
		for (uint_fast64_t i = 0; i < upper_bound; i++)
			if (!loc_table->locations[i].keep_flag) {
				while (upper_bound && !loc_table->locations[upper_bound - 1].keep_flag)
					upper_bound--;
				if (upper_bound > 0 && i < upper_bound) {
					struct loc_info temp = loc_table->locations[i];
					loc_table->locations[i] = loc_table->locations[upper_bound - 1];
					loc_table->locations[upper_bound - 1] = temp;
				}
				else
					break;
			}
		loc_table->loc_entries = upper_bound;
	}
}

void loc_table_print(struct loc_table* loc_table, const uint64_t chunk_loc) {
	uint64_t low = 0;
	uint64_t high = loc_table->loc_entries;

	do {
		uint64_t middle = (high - low) / 2 + low;
		if (chunk_loc > loc_table->locations[middle].chunk_loc)
			low = middle;
		else
			high = middle;
	} while (high - low > 1);

	printf("ROW: %" PRIu64 ", COL: %" PRIu64, loc_table->locations[low].src_row, loc_table->locations[low].src_col);
	if (loc_table->locations[low].file)
		printf(" in %s.", loc_table->locations[low].file);
	else
		printf(" from the interactive REPL.");
}

void debug_print_scanner(struct scanner scanner) {
	if (scanner.pos >= scanner.size)
		scanner.pos = scanner.size - 1;
	while (--scanner.pos)
		if (scanner.source[scanner.pos] == '\n') {
			scanner.pos++;
			break;
		}

	while (scanner.pos < scanner.size && scanner.source[scanner.pos] != '\n')
		printf("%c", scanner.source[scanner.pos++]);
	printf("\nROW: %" PRIu64 ", COL: %" PRIu64, scanner.row, scanner.col);
	if (scanner.file)
		printf(" in %s", scanner.file);
	else
		printf(" from the interactive REPL.");
}

static void print_instruction_dump(struct chunk* chunk, uint32_t* indent) {
	printf("%" PRIu64, chunk->pos);
	enum op_code op = chunk_read_opcode(chunk);

	for (uint32_t i = 0; i < *indent; i++)
		printf("\t");

	switch (op)
	{
	case MACHINE_LOAD_VAR:
		printf("LOAD VAR, id:%"PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_STORE_VAR:
		printf("STORE VAR, id:%"PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_LOAD_CONST:
		printf("LOAD CONST, const:");
		print_value(chunk_read_value(chunk), 0);
		break;
	case MACHINE_EVAL_BIN_OP:
		printf("EVAL BIN OP, op: %d", chunk_read_bin_op(chunk));
		break;
	case MACHINE_EVAL_UNI_OP:
		printf("EVAL UNI OP, op: %d", chunk_read_uni_op(chunk));
		break;
	case MACHINE_END_SKIP:
		printf("END_SKIP");
		(*indent)--;
		break;
	case MACHINE_MARK:
		printf("MARK");
		break;
	case MACHINE_RETURN_GOTO:
		printf("RETURN");
		break;
	case MACHINE_GOTO:
		printf("GOTO, id:%" PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_GOTO_AS:
		printf("GOTO AS, id:%" PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_LABEL:
		chunk_read_ulong(chunk);
		printf("LABEL, id:%" PRIu64, chunk_read_ulong(chunk));
		(*indent)++;
		break;
	case MACHINE_COND_SKIP:
		chunk_read_ulong(chunk);
		printf("COND SKIP");
		(*indent)++;
		break;
	case MACHINE_COND_RETURN:
		printf("COND RETURN");
		break;
	case MACHINE_FLAG:
		printf("SET FLAG");
		break;
	case MACHINE_RESET_FLAG:
		printf("RESET FLAG");
		break;
	case MACHINE_FLAG_SKIP:
		chunk_read_ulong(chunk);
		printf("FLAG SKIP");
		(*indent)++;
		break;
	case MACHINE_NEW_FRAME:
		printf("NEW FRAME");
		break;
	case MACHINE_CLEAN:
		printf("GC CLEAN");
		break;
	case MACHINE_BUILD_COL:
		printf("BUILD COL, size:%" PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_BUILD_PROTO: {
		printf("BUILD RECORD-PROTO, id:%" PRIu64, chunk_read_ulong(chunk));
		uint64_t properties = chunk_read_ulong(chunk);
		printf(", properties:%" PRIu64, properties);
		while (properties--)
			printf(", id:%" PRIu64, chunk_read_ulong(chunk));
		break;
	}
	case MACHINE_BUILD_RECORD:
		printf("BUILD RECORD, proto-id:%" PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_INHERIT_REC:
		printf("INHERIT RECORD, child:%" PRIu64 ", parent:%"PRIu64, chunk_read_ulong(chunk), chunk_read_ulong(chunk));
		break;
	case MACHINE_SET_INDEX:
		printf("SET INDEX");
		break;
	case MACHINE_GET_INDEX:
		printf("GET INDEX");
		break;
	case MACHINE_GET_PROPERTY:
		printf("GET PROPERTY, property:%" PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_SET_PROPERTY:
		printf("SET PROPERTY, property:%" PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_TRACE:
		printf("GC SET TRACE, args: %" PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_POP:
		printf("POP EVAL");
		break;
	case MACHINE_CALL_EXTERN:
		printf("CALL EXTERN, args:% " PRIu64 " id:%" PRIu64, chunk_read_ulong(chunk), chunk_read_ulong(chunk));
		break;
	case MACHINE_END:
		printf("END-DUMP");
		break;
	}

}

void debug_print_dump(struct chunk chunk) {
	uint64_t old_pos = chunk.pos;
	chunk.pos = 0;
	uint32_t indent = 1;
	chunk.last_code = -1;

	int pos_flag = 1;

	while (chunk.last_code != MACHINE_END) {
		print_instruction_dump(&chunk, &indent);
		if (chunk.pos >= old_pos && chunk.last_code != MACHINE_END && pos_flag) {
			printf(" <<< IP");
			pos_flag = 0;
		}
		printf("\n");
	}
}

void debug_print_trace(struct machine* machine, struct loc_table* table, uint64_t last_pos) {
	for (uint_fast64_t i = 0; i < machine->positions; i++) {
		printf("in ");
		loc_table_print(table, machine->position_stack[i]);
		printf("\n");
	}
	printf("\t");
	loc_table_print(table, last_pos);
}