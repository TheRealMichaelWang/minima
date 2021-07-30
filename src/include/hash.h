#pragma once

#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include "value.h"

const uint64_t hash(const char* str, const uint64_t len);
const uint64_t combine_hash(const uint64_t hash_a, const uint64_t hash_b);

const uint64_t value_hash(struct value value);

#endif // !HASH_H