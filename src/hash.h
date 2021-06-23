#pragma once

#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include "value.h"

uint64_t hash(const char* str, uint64_t len);
uint64_t combine_hash(uint64_t hash_a, uint64_t hash_b);

uint64_t value_hash(struct value value);

#endif // !HASH_H