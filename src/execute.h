#ifndef MARCEL_EXEC_H
#define MARCEL_EXEC_H

#include "ds/proc.h" // proc

// Builtin function
typedef int (*proc_func)(proc const*);

int launch_job(job *j);
_Bool initialize_builtins(void);


#endif
