#include <setjmp.h>
#include <stdio.h>
#include <signal.h>

#include <sys/wait.h>

#include "msh_macros.h"

typedef void (*sigfunc)(int);

sigjmp_buf ctrl_c_buf;

static void handle_sigint(int signo)
{
    puts("");
    siglongjmp(ctrl_c_buf, 1);
}

static void handle_sigchld(int signo)
{
    waitpid(-1, NULL, WNOHANG | WUNTRACED);
}

int signals[] = { SIGINT, SIGCHLD};
sigfunc funcs[]   = { &handle_sigint, &handle_sigchld };


int setup_signals(void)
{
    size_t len = Arr_len(signals);
    struct sigaction act = {0};
    if (sigemptyset(&act.sa_mask) == -1) return 1;

    for (size_t i = 0; i < len; i++) {
        act.sa_handler = funcs[i];
        if (sigaction(signals[i], &act, NULL) == -1) return 1;
    }
    return 0;

}
