#pragma once

#ifndef GETSET_H
#define GETSET_H

#include "machine.h"
#include "chunk.h"

const int get_index(struct machine* machine);
const int set_index(struct machine* machine);

#endif // !GETSET_H