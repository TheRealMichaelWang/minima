#include "object.h"
#include "hash.h"

uint64_t hash(const char* str, uint64_t len) {
    uint32_t hash = 5381; //DO NOT CHANGE THE HASH SIZE, otherwise pre-computed hashes WILL NOT WORK
    for (uint64_t i = 0; i < len; i++)
        hash = hash * 33 + str[i];
    return hash;
}

uint64_t combine_hash(uint64_t hash_a, uint64_t hash_b) {
    return (5381 + hash_a) << 5 + hash_b;
}

uint64_t value_hash(struct value value) {
    if (value.type == VALUE_TYPE_NULL)
        return 0;
    else if (value.type == VALUE_TYPE_CHAR)
        return combine_hash(VALUE_TYPE_CHAR, value.payload.character);
    else if (value.type == VALUE_TYPE_NUM)
        return combine_hash(VALUE_TYPE_NUM, value.payload.numerical);
    else {
        struct object object = value.payload.object;
        uint64_t size, hash;
        struct value** children = object_get_children(&object, &size);
        hash = value.type;
        for (uint_fast64_t i = 0; i < size; i++)
            combine_hash(hash, value_hash(*children[i]));
        return hash;
    }
}