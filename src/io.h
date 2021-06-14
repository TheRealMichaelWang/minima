#pragma once

#ifndef IO_H
#define IO_H

#include "value.h"

void print_value(struct value* value, const int print_mode);
struct value* print(struct value** argv, unsigned int argc);
struct value* get_input(struct value** argv, unsigned int argc);
struct value* get_length(struct value** argv, unsigned int argc);

#endif // !IO_H