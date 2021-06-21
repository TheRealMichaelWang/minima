#include "hash.h"

uint64_t hash(const char* str, uint64_t len) {
    uint32_t hash = 5381;
    for (uint64_t i = 0; i < len; i++)
        hash = hash * 33 + str[i];
    return hash;
}

uint64_t combine_hash(uint64_t hash_a, uint64_t hash_b) {
    return (5381 + hash_a) << 5 + hash_b;
}