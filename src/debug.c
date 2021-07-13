#include <stdio.h>
#include <inttypes.h>
#include "include/runtime/opcodes.h"
#include "include/io.h"
#include "include/debug.h"

void debug_print_scanner(struct scanner scanner) {
	if (scanner.pos >= scanner.size)
		scanner.pos = scanner.size - 1;
	scanner.pos--;
	uint32_t pos = scanner.pos - 1;
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

static void print_instruction_dump(struct chunk* chunk, uint32_t* indent) {
	printf("%" PRIu64, chunk->pos);
	char op_code = chunk_read(chunk);

	for (uint32_t i = 0; i < *indent; i++)
		printf("\t");

	switch (op_code)
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
		printf("GOTO, id:%" PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_GOTO_AS:
		printf("GOTO AS, id:%" PRIu64, chunk_read_ulong(chunk));
		break;
	case MACHINE_LABEL:
		printf("LABEL, id:%" PRIu64, chunk_read_ulong(chunk));
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
		printf("GC SET TRACE");
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

	while (chunk.last_code != MACHINE_END) {
		print_instruction_dump(&chunk, &indent);
		if (chunk.pos == old_pos && chunk.last_code != MACHINE_END)
			printf(" <<< IP");
		printf("\n");
	}
}