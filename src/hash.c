#include "hash.h"

unsigned long hash(const char* str, unsigned long len) {
    unsigned long hash = 5381;
    for (unsigned long i = 0; i < len; i++)
        hash = hash * 33 + str[i];
    return hash;
}

unsigned long combine(unsigned long hash_a, unsigned long hash_b) {
    return 5381 + hash_a * 33 + hash_b;
}