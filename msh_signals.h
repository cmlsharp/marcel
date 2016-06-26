#ifndef MSH_SIG_H
#define MSH_SIG_H

#include <setjmp.h>

extern sigjmp_buf ctrl_c_buf;

int setup_signals(void);
void sigint_resume(void);

#define Sigint_reentry() do {while (sigsetjmp(ctrl_c_buf, 1));} while (0)

#endif
