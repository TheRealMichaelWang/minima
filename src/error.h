#pragma once

#ifndef ERROR_H

enum error {
	error_insufficient_memory,
	error_insufficient_evals,
	error_insufficient_calls,

	error_label_redefine,
	error_label_undefined,
	error_record_redefine,
	error_record_undefined,
	error_property_redefine,
	error_property_undefined,

	error_unnexpected_type,
	error_index_out_of_range,
	error_stack_overflow,
	error_variable_undefined,

	error_unrecognized_tok,
	error_unrecognized_control_seq,
	error_unexpected_char,
	error_unexpected_tok,
	error_cannot_open_file,
};

#endif // !ERROR_H
