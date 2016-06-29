#define _XOPEN_SOURCE
#include <setjmp.h>
#include <stdio.h>
#include <signal.h>

#include <sys/wait.h>

#include "msh_children.h"
#include "msh_macros.h"

int signals[] = { SIGINT, SIGCHLD };

static void handle_signals(int signo)
{
    pid_t p = 0;
    switch (signo) {
    case SIGINT:
        exit_code = 127;
        SIG_IGN;
        break;
    case SIGCHLD:
        for (;;) {
            p = waitpid(-1, NULL, WNOHANG | WUNTRACED);
            if (p == 0) { // Active child generates  SIGCHLD but background process still running
                break;
            } else if (p == -1) { // No children (active child already terminated but SIGCHLD still thrown)
                reset_active_child();
                break;
            } else if (p == get_active_child()) { // Active child not cleaned up (usually because it is signaled). Not sure why this happens
                reset_active_child();
                continue;
            }

            size_t job_num = del_bkg_proc(p);
            printf("[%zu] completed\n", job_num);
        }
        break;
    }

}


int setup_signals(void)
{
    size_t len = Arr_len(signals);
    struct sigaction act = {0};
    if (sigemptyset(&act.sa_mask) == -1) return 1;

    act.sa_handler = &handle_signals;
    for (size_t i = 0; i < len; i++) {
        if (sigaction(signals[i], &act, NULL) == -1) return 1;
    }

    return 0;
}
