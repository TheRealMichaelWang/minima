#pragma once

#ifndef MAP_H
#define MAP_H

#define MAP_MAX_BUCKETS 250

struct bucket {
	unsigned long hash;
	const void* value;
	struct bucket* next;
};

struct map {
	unsigned int elem_size;
	struct bucket* hash_set[MAP_MAX_BUCKETS];
};

void init_map(struct map* map, const unsigned int elem_size);

void free_map(struct map* map);

void emplace(struct map* map, const unsigned long key, const void* element);
const void* retrieve(struct map* map, const unsigned long key);

#endif // !MAP_H
