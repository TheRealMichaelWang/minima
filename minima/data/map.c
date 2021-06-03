#include <string.h>
#include <stdlib.h>
#include "map.h"

void init_map(struct map* map, const unsigned int elem_size) {
	map->elem_size = elem_size;
	for (unsigned char i = 0; i < MAP_MAX_BUCKETS; i++)
		map->hash_set[i] = NULL;
}

void free_map(struct map* map) {
	for (unsigned char i = 0; i < MAP_MAX_BUCKETS; i++) {
		struct bucket* current_bucket = map->hash_set[i];
		while (current_bucket != NULL)
		{
			free(current_bucket->value);
			struct bucket* old_bucket = current_bucket;
			current_bucket = current_bucket->next;
			free(old_bucket);
		}
	}
}

void emplace(struct map* map, const unsigned long key, const void* element) {
	struct bucket** current_bucket = &map->hash_set[key % MAP_MAX_BUCKETS];
	while (*current_bucket != NULL)
	{
		if ((*current_bucket)->hash == key)
			break;
		current_bucket = &(*current_bucket)->next;
	}
	if (*current_bucket == NULL) {
		*current_bucket = malloc(sizeof(struct bucket));
		if (*current_bucket == NULL)
			exit(1);
		(*current_bucket)->hash = key;
		(*current_bucket)->value = malloc(map->elem_size);
		if ((*current_bucket)->value == NULL)
			exit(1);
		(*current_bucket)->next = NULL;
	}
	memcpy((*current_bucket)->value, element, map->elem_size);
}

const void* retrieve(struct map* map, const unsigned long key) {
	struct bucket* current_bucket = map->hash_set[key % MAP_MAX_BUCKETS];
	while (current_bucket != NULL)
	{
		if (current_bucket->hash == key)
			return current_bucket->value;
		current_bucket = current_bucket->next;
	}
	return NULL;
}