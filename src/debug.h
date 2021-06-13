#pragma once

#ifndef DEBUG_H
#define DEBUG_H

#include "scanner.h"
#include "chunk.h"

void print_last_line(struct scanner scanner);
void print_dump(struct chunk chunk);

#endif // !DEBUG_H