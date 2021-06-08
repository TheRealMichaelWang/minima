#pragma once

#ifndef BUITLINS_H
#define BUILTINS_H

struct builtin_register {
	struct builtin_procedure {
		unsigned long id;
		struct value* (*delegate)(struct value** argv, unsigned int argc);

		struct builtin_procedure* next;
	}** procedures;
};

void init_builtin_register(struct builtin_register* builtin_register);
void free_builtin_register(struct builtin_register* builtin_register);

const int declare_builtin_proc(struct builtin_register* builtin_register, unsigned long id, struct value* (*delegate)(struct value** argv, unsigned int argc));

struct value* invoke_builtin(struct builtin_register* builtin_register, unsigned long id, struct value** argv, unsigned int argc);

#endif // !BUITLINS_H