#include "hash.h"

unsigned long hash(const char* str, unsigned long len) {
    unsigned long hash = 5381;
    for (unsigned long i = 0; i < len; i++)
        hash = hash * 33 + str[i];
    return hash;
}