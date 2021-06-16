#ifndef STDLIB_H
#define STDLIB_H

#include "value.h"

struct value* builtin_print(struct value** argv, unsigned int argc);
struct value* builtin_print_line(struct value** argv, unsigned int argc);
struct value* builtin_system_cmd(struct value** argv, unsigned int argc);
struct value* builtin_random(struct value** argv, unsigned int argc);

struct value* builtin_get_input(struct value** argv, unsigned int argc);
struct value* builtin_get_length(struct value** argv, unsigned int argc);

#endif // !STDLIB_H