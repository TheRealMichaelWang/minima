#include <stdio.h>
#include "machine.h"
#include "io.h"
#include "debug.h"

void debug_print_scanner(struct scanner scanner) {
	if (scanner.pos >= scanner.size)
		scanner.pos = scanner.size - 1;
	scanner.pos--;
	unsigned int pos = scanner.pos - 1;
	while (scanner.pos != 0)
	{
		if (scanner.source[scanner.pos] == '\n') {
			scanner.pos++;
			break;
		}
		scanner.pos--;
	}

	while (scanner.pos < scanner.size && scanner.source[scanner.pos] != '\n')
		printf("%c", scanner.source[scanner.pos++]);
	printf("\n");
	while (pos--)
		printf(" ");
	printf("^");
}

void print_instruction_dump(struct chunk* chunk, unsigned int* indent) {
	printf("%d", chunk->pos);
	char op_code = chunk_read(chunk);

	for (unsigned int i = 0; i < *indent; i++)
		printf("\t");

	switch (op_code)
	{
	case MACHINE_LOAD_VAR:
		printf("LOAD VAR, id:%d", chunk_read_ulong(chunk));
		break;
	case MACHINE_STORE_VAR:
		printf("STORE VAR, id:%d", chunk_read_ulong(chunk));
		break;
	case MACHINE_LOAD_CONST:
		printf("LOAD CONST, const:");
		print_value(chunk_read_value(chunk), 0);
		break;
	case MACHINE_EVAL_BIN_OP:
		printf("EVAL BIN OP, op: %d", chunk_read(chunk));
		break;
	case MACHINE_EVAL_UNI_OP:
		printf("EVAL UNI OP, op: %d", chunk_read(chunk));
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
		printf("GOTO, id:%d", chunk_read_ulong(chunk));
		break;
	case MACHINE_GOTO_AS:
		printf("GOTO AS, id:%d", chunk_read_ulong(chunk));
		break;
	case MACHINE_LABEL:
		printf("LABEL, id:%d", chunk_read_ulong(chunk));
		(*indent)++;
		break;
	case MACHINE_COND_SKIP:
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
		printf("BUILD COL, size:%d", chunk_read_ulong(chunk));
		break;
	case MACHINE_BUILD_PROTO: {
		printf("BUILD RECORD-PROTO, id:%d", chunk_read_ulong(chunk));
		unsigned long properties = chunk_read_ulong(chunk);
		printf(", properties:%d", properties);
		while (properties--)
			printf(", id:%d", chunk_read_ulong(chunk));
		break;
	}
	case MACHINE_BUILD_RECORD:
		printf("BUILD RECORD, proto-id:%d", chunk_read_ulong(chunk));
		break;
	case MACHINE_SET_INDEX:
		printf("SET INDEX");
		break;
	case MACHINE_GET_INDEX:
		printf("GET INDEX");
		break;
	case MACHINE_GET_PROPERTY:
		printf("GET PROPERTY, property:%d", chunk_read_ulong(chunk));
		break;
	case MACHINE_SET_PROPERTY:
		printf("SET PROPERTY, property:%d", chunk_read_ulong(chunk));
		break;
	case MACHINE_TRACE:
		printf("GC SET TRACE");
		break;
	case MACHINE_POP:
		printf("POP EVAL");
		break;
	case MACHINE_CALL_EXTERN:
		printf("CALL EXTERN, args:%d id:%d", chunk_read_ulong(chunk), chunk_read_ulong(chunk));
		break;
	}

}

void debug_print_dump(struct chunk chunk) {
	unsigned long old_pos = chunk.pos;
	chunk.pos = 0;
	unsigned int indent = 1;
	while (!end_chunk(&chunk)) {
		print_instruction_dump(&chunk, &indent);
		if (chunk.pos == old_pos)
			printf(" <<< IP");
		printf("\n");
	}
}