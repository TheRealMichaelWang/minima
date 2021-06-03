#include <stdlib.h>
#include <string.h>
#include "list.h"

void init_list(struct list* list, const unsigned int elem_size) {
	list->elem_size = elem_size;
	list->size = 0;
	list->head = NULL;
	list->tail = NULL;
}

void free_list(struct list* list) {
	struct node* current = list->head;
	while (current != NULL)
	{
		free(current->value);
		struct node* old = current;
		current = current->next;
		free(old);
	}
}

void push_back(struct list* list, const void* element) {
	struct node* toadd = malloc(sizeof(struct node));
	if (toadd == NULL)
		exit(1);
	toadd->value = malloc(list->elem_size);
	if (toadd->value == NULL)
		exit(1);
	memcpy(toadd->value, element, list->elem_size);
	if (list->size == 0) {
		toadd->prev = NULL;
		toadd->next = NULL;
		list->head = toadd;
		list->tail = toadd;
	}
	else {
		toadd->prev = list->tail;
		toadd->next = NULL;
		list->tail->next = toadd;
		list->tail = toadd;
	}
	list->size++;
}

void push_front(struct list* list, const void* element) {
	struct node* toadd = malloc(sizeof(struct node));
	if (toadd == NULL)
		exit(1);
	toadd->value = malloc(list->elem_size);
	if (toadd->value == NULL)
		exit(1);
	memcpy(toadd->value, element, list->elem_size);
	if (list->size == 0) {
		toadd->prev = NULL;
		toadd->next = NULL;
		list->head = toadd;
		list->tail = toadd;
	}
	else {
		toadd->next = list->head;
		toadd->prev = NULL;
		list->head->prev = toadd;
		list->head = toadd;
	}
	list->size++;
}

void* pop_front(struct list* list) {
	void* elem = NULL;
	if (list->size == 0)
		return NULL;
	else if (list->size == 1) {
		elem = list->head->value;
		free(list->head);
		list->head = NULL;
		list->tail = NULL;
	}
	else {
		elem = list->head->value;
		struct node* next = list->head->next;
		next->prev = NULL;
		free(list->head);
		list->head = next;
	}
	list->size--;
	return elem;
}

void* pop_back(struct list* list) {
	void* elem = NULL;
	if (list->size == 0)
		return NULL;
	else if (list->size == 1) {
		elem = list->head->value;
		free(list->head);
		list->head = NULL;
		list->tail = NULL;
	}
	else {
		elem = list->head->value;
		struct node* prev = list->tail->prev;
		prev->next = NULL;
		free(list->tail);
		list->tail = prev;
	}
	list->size--;
	return elem;
}