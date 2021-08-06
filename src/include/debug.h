#pragma once

#ifndef DEBUG_H
#define DEBUG_H

#include "scanner.h"
#include "compiler.h"
#include "chunk.h"
#include "machine.h"

struct loc_table {
	struct loc_info {
		uint64_t chunk_loc, src_row, src_col;
		const char* file;
		int offset_flag, keep_flag;
	}* locations;

	char** to_free;
	const char** file_stack;

	uint64_t loc_entries, alloced_locs, files, alloced_files, current_file, global_offset;
	int finilazed_flag;
};

const int init_loc_table(struct loc_table* loc_table, char* initial_file);
void free_loc_table(struct loc_table* loc_table);

const int loc_table_include(struct loc_table* loc_table, char* file);
void loc_table_uninclude(struct loc_table* loc_table);

const int loc_table_insert(struct loc_table* loc_table, struct compiler* compiler, struct chunk_builder* builder);
void loc_table_finalize(struct loc_table* loc_table, struct compiler* compiler, int keep);
void loc_table_dispose(struct loc_table* loc_table);
void loc_table_print(struct loc_table* loc_table, const uint64_t chunk_loc);

void debug_print_scanner(struct scanner scanner);
void debug_print_dump(struct chunk chunk);
void debug_print_trace(struct machine* machine, struct loc_table* table, uint64_t last_pos);

#endif // !DEBUG_H