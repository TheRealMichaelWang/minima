#include "include/runtime/object/object.h"
#include "include/hash.h"

const uint64_t hash(const char* str, const uint64_t len) {
    uint64_t hash = 5381; //DO NOT CHANGE THE HASH SIZE, otherwise pre-computed hashes WILL NOT WORK
    for (uint64_t i = 0; i < len; i++)
        hash = (hash << 5) + hash + str[i];
    return hash;
}

const uint64_t combine_hash(const uint64_t hash_a, const uint64_t hash_b) {
    uint64_t hash = 5381;
    hash = (hash << 5) + hash + hash_a;
    hash = (hash << 5) + hash + hash_b;
    return hash;
}

const uint64_t value_hash(struct value value) {
    if (value.type == VALUE_TYPE_NULL)
        return 0;
    else if (value.type == VALUE_TYPE_CHAR)
        return combine_hash(VALUE_TYPE_CHAR, value.payload.character);
    else if (value.type == VALUE_TYPE_NUM)
        return combine_hash(VALUE_TYPE_NUM, value.payload.numerical);
    else {
        struct object object = value.payload.object;
        uint64_t size, hash;
        struct value**children = object_get_children(&object, &size);
        hash = value.type;
        for (uint_fast64_t i = 0; i < size; i++)
            hash = combine_hash(hash, value_hash(*children[i]));
        return hash;
    }
}