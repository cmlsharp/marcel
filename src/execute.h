#ifndef MARCEL_EXEC_H
#define MARCEL_EXEC_H

#include "ds/cmd.h" // cmd

// Builtin function
typedef int (*cmd_func)(cmd const*);

int launch_job(job *j);
_Bool initialize_builtins(void);


#endif
