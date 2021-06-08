#include <stdlib.h>
#include "builtins.h"

#define MAX_SIZE 255

void init_builtin_register(struct builtin_register* builtin_register) {
	builtin_register->procedures = calloc(MAX_SIZE, sizeof(struct builtin_procedure*));
}

void free_builtin_register(struct builtin_register* builtin_register) {
	for (unsigned char i = 0; i < MAX_SIZE; i++) {
		struct builtin_procedure* procedure = builtin_register->procedures[i];
		while (procedure != NULL)
		{
			struct builtin_procedure* old = procedure;
			procedure = procedure->next;
			free(old);
		}
	}
	free(builtin_register->procedures);
}

const int declare_builtin_proc(struct builtin_register* builtin_register, unsigned long id, struct value* (*delegate)(struct value** argv, unsigned int argc)) {
	struct builtin_procedure** procedure = &builtin_register->procedures[id % MAX_SIZE];
	while (*procedure != NULL)
	{
		if ((*procedure)->id == id)
			return 0;
		procedure = &(*procedure)->next;
	}
	*procedure = malloc(sizeof(struct builtin_procedure));
	if (*procedure == NULL)
		return 0;
	(*procedure)->id = id;
	(*procedure)->delegate = delegate;
	(*procedure)->next = NULL;
	return 1;
}

struct value* invoke_builtin(struct builtin_register* builtin_register, unsigned long id, struct value** argv, unsigned int argc) {
	struct builtin_procedure* current = builtin_register->procedures[id % MAX_SIZE];
	while (current != NULL)
	{
		if (current->id = id) {
			return (*current->delegate)(argv, argc);
		}
		current = current->next;
	}
	return NULL;
}