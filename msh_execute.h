#ifndef MSH_EXEC_H
#define MSH_EXEC_H

#include "msh.h"

typedef int (*cmd_func)(cmd const*);

int run_cmd(cmd *const c);
int initialize_internals(void);
void cleanup_internals(void);

#endif
