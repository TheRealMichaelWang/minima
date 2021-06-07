#include <stdlib.h>
#include "labels.h"

#define MAX_LABEL_BUCKETS 255

void init_label_cache(struct label_cache* label_cache) {
	label_cache->buckets = calloc(MAX_LABEL_BUCKETS, sizeof(struct label_bucket*));
}

void free_label_cache(struct label_cache* label_cache) {
	for (unsigned char i = 0; i < MAX_LABEL_BUCKETS; i++) {
		struct label_bucket* bucket = label_cache->buckets[i];
		while (bucket != NULL)
		{
			struct label_bucket* old = bucket;
			bucket = bucket->next;
			free(old);
		}
	}
	free(label_cache->buckets);
}

int insert_label(struct label_cache* label_cache, unsigned long id, unsigned long pos) {
	struct label_bucket** bucket = &label_cache->buckets[id % MAX_LABEL_BUCKETS];
	while (*bucket != NULL)
	{
		if ((*bucket)->id == id)
			return 0;
		bucket = &(*bucket)->next;
	}
	*bucket = malloc(sizeof(struct label_bucket));
	if (*bucket == NULL)
		return 0;
	(*bucket)->next = NULL;
	(*bucket)->id = id;
	(*bucket)->pos = pos;
	return 1;
}

unsigned long retrieve_pos(struct label_cache* label_cache, unsigned long id) {
	struct label_bucket* bucket = label_cache->buckets[id % MAX_LABEL_BUCKETS];
	while (bucket != NULL)
	{
		if (bucket->id == id)
			return bucket->pos;
		bucket = bucket->next;
	}
	return 0;
}