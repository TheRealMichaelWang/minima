#define _CRT_SECURE_NO_DEPRECATE
//buffer security is imoortant, but not as much as portability

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "include/debug.h"
#include "include/hash.h"
#include "include/compiler/compiler.h"
#include "include/runtime/opcodes.h"
#include "include/runtime/machine.h"

static struct machine machine;

static uint32_t block_check(const char* src, uint32_t length) {
	uint32_t blocks = 0;
	for (uint32_t i = 0; i < length; i++) {
		if (src[i] == '{')
			blocks++;
		else if (src[i] == '}')
			blocks--;
	}
	return blocks;
}

int main(uint32_t argc, char** argv) {
	if (!init_machine(&machine))
		exit(EXIT_FAILURE);
	struct compiler compiler;

	if (argc > 1) {
		FILE* infile = fopen(argv[1], "rb");
		if (!infile) {
			printf("Minima couldn't open the file, \"%s\".", argv[1]);
			exit(EXIT_FAILURE);
		}
		fseek(infile, 0, SEEK_END);
		uint64_t fsize = ftell(infile);
		fseek(infile, 0, SEEK_SET);
		char* source = malloc(fsize + 1);
		if (source == NULL) {
			fclose(infile);
			exit(EXIT_FAILURE);
		}
		fread(source, 1, fsize, infile);
		fclose(infile);
		source[fsize] = 0;

		init_compiler(&compiler, source);

		compiler.imported_file_hashes[compiler.imported_files++] = hash(argv[1], strlen(argv[1]));

		if (!compile(&compiler, 0)) {
			printf("\n***Syntax Error***\n");
			error_info(compiler.last_err);
			printf("\n\n");
			debug_print_scanner(compiler.scanner);
		}
		else {
			struct chunk source_chunk = compiler_get_chunk(&compiler);
			enum error err = machine_execute(&machine, &source_chunk);
			if (err) {
				printf("\n***Runtime Error***\n");
				error_info(err);
				printf("\n\nDUMP: \n");
				debug_print_dump(source_chunk);
			}
		}
		free(source);

		if (argc > 2 && !strcmp(argv[2], "--debug")) {
			printf("\nPress ENTER to exit.");
			getchar();
		}
	}
	else {
		printf("North-Hollywood Minima, version 1.0\n");
		printf("Written by Michael Wang in 2021\n\n");
		printf("Type \"dump\" to produce a bytecode dump of your current program. Type \"quit\" to exit.\n");
		printf("READY\n");

		struct chunk_builder global_build; //contains our source
		uint64_t ip = 0;
		uint32_t imported_files = 0;
		init_chunk_builder(&global_build);
		chunk_write(&global_build, MACHINE_NEW_FRAME);

		while (1)
		{
			char src_buf[4096];
			uint32_t index = 0;
			printf("\n");
			while (block_check(src_buf, index) || !index)
			{
				printf(">>> ");
				while (scanf_s("%c", &src_buf[index], 1)) {
					if (src_buf[index] == '\n')
						break;
					index++;
				}
				src_buf[index++] = '\n';
			}
			src_buf[index] = 0;

			if (!strcmp(src_buf, "quit\n"))
				break;
			else if (!strcmp(src_buf, "dump\n")) {
				printf("\nGLOBAL DUMP:\n");
				struct chunk global_chunk = build_chunk(&global_build);
				global_chunk.pos = ip;
				debug_print_dump(global_chunk);
			}
			else {
				init_compiler(&compiler, src_buf);
				compiler.imported_files = imported_files;

				if (!compile(&compiler, 1)) {
					printf("\n***Syntax Error***\n");
					error_info(compiler.last_err);
					printf("\n\n");
					debug_print_scanner(compiler.scanner);
					printf("\n");
				}
				else {
					struct chunk new_chunk = compiler_get_chunk(&compiler);
					chunk_write_chunk(&global_build, new_chunk, 1);
					imported_files = compiler.imported_files;

					struct chunk global_chunk = build_chunk(&global_build);
					chunk_jump_to(&global_chunk, ip);
					chunk_read(&global_chunk);

 					enum error err = machine_execute(&machine, &global_chunk);
					if (err) {
						printf("\n***Runtime Error***\n");
						error_info(err);
						printf("\n\nGLOBAL DUMP:\n");
						debug_print_dump(global_chunk);

						global_build.size = ip;
						machine_reset(&machine);
					}
					else
						ip = global_chunk.pos;
				}
			}
		}
		struct chunk global_chunk = build_chunk(&global_build);
		free_chunk(&global_chunk);
	}
	free_machine(&machine);
   	exit(EXIT_SUCCESS);
}