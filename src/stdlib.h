#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>
#include "value.h"

//API to facilitate the declaration of built-in procedures which can be called using the "extern" keyword within the Minima language
#define DECL_BUILT_IN(METHOD_NAME) struct value* METHOD_NAME(struct value** argv, uint32_t argc)

//prints values
DECL_BUILT_IN(builtin_print); 

//calls builtin_print and prints a newline afterwards
DECL_BUILT_IN(builtin_print_line); 

//runs a system command
DECL_BUILT_IN(builtin_system_cmd);

//generates a random number from 0-1
DECL_BUILT_IN(builtin_random); 

//gets user input
DECL_BUILT_IN(builtin_get_input); 

//gets the length of an array
DECL_BUILT_IN(builtin_get_length); 

#endif // !STDLIB_H