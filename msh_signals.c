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
        while ((p = waitpid(-1, NULL, WNOHANG | WUNTRACED))) {
            if (p == get_active_child()) {
                reset_active_child();
                continue;
            }

            if (p == -1) {
                reset_active_child();
                break;
            }

            size_t job_num = del_bkg_proc(p);
            printf("[%zu] completed\n", job_num);
        }
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
