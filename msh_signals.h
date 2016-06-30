#ifndef MSH_SIG_H
#define MSH_SIG_H

#include <setjmp.h>

// This variable is only accessed via the two macros beolow
extern sigjmp_buf _sigbuf;

#define SIGINT_EXIT_CODE -1
#define Sigint_init_reentry() do { sigsetjmp(_sigbuf, 1); } while (0)
#define Sigint_reentry() do { while (sigsetjmp(_sigbuf, 1)); } while (0)

int setup_signals(void);

#endif
