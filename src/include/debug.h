#pragma once

#ifndef DEBUG_H
#define DEBUG_H

#include "include/compiler/scanner.h"
#include "include/compiler/chunk.h"

void debug_print_scanner(struct scanner scanner);
void debug_print_dump(struct chunk chunk);

#endif // !DEBUG_H