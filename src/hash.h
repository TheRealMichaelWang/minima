#pragma once

#ifndef HASH_H
#define HASH_H

#include <stdint.h>

uint64_t hash(const char* str, uint64_t len);
uint64_t combine_hash(uint64_t hash_a, uint64_t hash_b);

#endif // !HASH_H