#include <stdio.h>
#include <stdlib.h>
#include "machine.h"
#include "operators.h"

int main() {
	struct machine mymachine;
	init_machine(&mymachine);

	struct chunk_builder builder;
	init_chunk_builder(&builder);

	struct value value_buf;

	write(&builder, MACHINE_LOAD_CONST);

	init_num(&value_buf, 10);
	write_value(&builder, value_buf);

	write(&builder, MACHINE_LOAD_CONST);

	init_num(&value_buf, 12);
	write_value(&builder, value_buf);

	write(&builder, MACHINE_EVAL_BIN_OP);
	write(&builder, OPERATOR_LESS);

	struct chunk to_execute = build_and_free(&builder);

	execute(&mymachine, &to_execute);
	free_machine(&mymachine);
	free_chunk(&to_execute);
}