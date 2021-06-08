#pragma once

#ifndef LABEL_H
#define LABEL_H

struct label_cache {
	struct label_bucket {
		unsigned long id;
		unsigned long pos;

		struct label_bucket* next;
	}** buckets;
};

void init_label_cache(struct label_cache* label_cache);
void free_label_cache(struct label_cache* label_cache);

int insert_label(struct label_cache* label_cache, unsigned long id, unsigned long pos);
unsigned long retrieve_pos(struct label_cache* label_cache, unsigned long id);

#endif // !LABEL_H
