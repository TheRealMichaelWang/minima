#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "compiler.h"

#undef _DEBUG
#define NO_DEBUG

unsigned int block_check(const char* src, unsigned int length) {
	unsigned int blocks = 0;
	for (unsigned int i = 0; i < length; i++) {
		if (src[i] == '{')
			blocks++;
		else if (src[i] == '}')
			blocks--;
	}
	return blocks;
}

int main(unsigned int argc, char** argv) {
	printf("North-Hollywood Minima, version 1.0\n");
	printf("Written by Michael Wang in 2021\n\n");

	struct machine machine;
	init_machine(&machine);

	struct compiler compiler;

	printf("Type \"quit\" to exit...\n");
	printf("READY\n");

	struct chunk_builder global_build; //contains our source
	unsigned long ip = 0;
	init_chunk_builder(&global_build);
	write(&global_build, MACHINE_NEW_FRAME);

	while (1)
	{
		char src_buf[4096];
		unsigned int index = 0;
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
			print_dump(global_chunk, 1);
		}
		else {
			init_compiler(&compiler, src_buf);

			if (!compile(&compiler, 1)) {
				printf("\n***Syntax Error***\nError No. %d\n", compiler.last_err);
				print_last_line(compiler.scanner);
				printf("\n");
			}
			else {
				struct chunk new_chunk = build_chunk(&compiler.chunk_builder);
				write_chunk(&global_build, new_chunk);

				struct chunk global_chunk = build_chunk(&global_build);
				global_chunk.pos = ip;

				int err = execute(&machine, &global_chunk);
				if (err) {
					printf("\n***Runtime Error***\nError No. %d\n", err);
					printf("\nERROR DUMP:\n");
					print_dump(new_chunk, 0);
					printf("\nGLOBAL DUMP:\n");
					print_dump(global_chunk, 1);

					global_build.size = ip;
					reset_stack(&machine);
				}
				else
					ip = global_chunk.pos;
			}
		}
	}

	free_machine(&machine);
	struct chunk global_chunk = build_chunk(&global_build);
	free_chunk(&global_chunk);

	return 0;
}