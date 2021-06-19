#ifndef STDLIB_H
#define STDLIB_H

#include "value.h"

#define DECL_BUILT_IN(METHOD_NAME) struct value* METHOD_NAME(struct value** argv, unsigned int argc)

DECL_BUILT_IN(builtin_print);
DECL_BUILT_IN(builtin_print_line);
DECL_BUILT_IN(builtin_system_cmd);
DECL_BUILT_IN(builtin_random);

DECL_BUILT_IN(builtin_get_input);
DECL_BUILT_IN(builtin_get_length);

#endif // !STDLIB_H