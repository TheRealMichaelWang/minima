#pragma once

#ifndef LIST_H
#define LIST_H

struct node {
	void* value;
	struct node* next;
	struct node* prev;
};

struct list {
	struct node* head;
	struct node* tail;
	
	unsigned int elem_size;
	unsigned long size;
};

void init_list(struct list* list, const unsigned int elem_size);
void free_list(struct list* list);

void push_back(struct list* list, const void* element);
void push_front(struct list* list, const void* element);

void* pop_front(struct list* list);
void* pop_back(struct list* list);

#endif // !LIST_H