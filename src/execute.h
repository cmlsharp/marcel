#ifndef MARCEL_EXEC_H
#define MARCEL_EXEC_H

#include "marcel.h" // cmd

// Builtin function
typedef int (*cmd_func)(cmd const*);

int run_cmd(cmd const *c);
int initialize_internals(void);

#endif
