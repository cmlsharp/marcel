// NOTE: Much of the below was inspired by the signal handling found in Z Shell
// (particularly the helper functions and signal queue)
#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdint.h>

#include <signal.h>

#include "jobs.h"
#include "macros.h"
#include "signals.h"

#if ((SIG_ATOMIC_MAX - 1) < 1024)
#define MAX_QUEUE_SIZE ((size_t) SIG_ATOMIC_MAX)
#else
#define MAX_QUEUE_SIZE ((size_t) 1024)
#endif


typedef struct sigstate {
    int sig;
    sigset_t mask;
} sigstate;

static void handler_sync(sigstate state);
static void handler_async(int signo);

// Buffer to allow jumps out of signal handler
sigjmp_buf _sigbuf;

sig_atomic_t volatile queue_front, queue_back;
sigstate volatile signal_queue[MAX_QUEUE_SIZE];

// Bitmask passed to signal handler
sig_atomic_t sig_flags;

void sig_handle(int sig)
{
    struct sigaction act = {0};
    sigemptyset(&act.sa_mask);
    act.sa_handler = handler_async;
    sigaction(sig, &act, NULL);

}


sigset_t sig_block(sigset_t new)
{
    sigset_t old;
    sigprocmask(SIG_BLOCK, &new, &old);
    return old;
}

// Remove signals in `new` from signal mask and return previous signal mask
sigset_t sig_unblock(sigset_t new)
{
    sigset_t old;
    sigprocmask(SIG_UNBLOCK, &new, &old);
    return old;
}

// Set signal mask, return previous signal mask
sigset_t sig_setmask(sigset_t new)
{
    sigset_t old;
    sigprocmask(SIG_SETMASK, &new, &old);
    return old;
}

// Ignore signal
void sig_ignore(int sig)
{
    signal(sig, SIG_IGN);
}

// Reset signal handler to default action
void sig_default(int sig)
{
    signal(sig, SIG_DFL);
}

// Signal queueing

void run_queued_signals(void)
{
    while (queue_front != queue_back) {
        queue_front = (queue_front + 1) % MAX_QUEUE_SIZE;
        handler_sync(signal_queue[queue_front]);
    }
}

static void handler_async(int signo)
{
    // Block all signals while we see about queueing
    sigset_t new;
    sigfillset(&new);
    sigset_t old = sig_block(new);

    int temp = ++queue_back % MAX_QUEUE_SIZE;
    Stopif(temp == queue_front, /* No action */, "Signal queue is full");
    if (temp != queue_front) {
        queue_back = temp;
        signal_queue[queue_back] = (sigstate) {
            .sig = signo, .mask = old
        };
    }

    if (sig_flags & WAITING_FOR_INPUT) {
        siglongjmp(_sigbuf, 1);
    }
    // Process automatically restores mask when handler returns
    // sig_setmask(old);

}

static void handler_sync(sigstate state)
{
    sigset_t old = sig_setmask(state.mask);
    switch (state.sig) {
    case SIGINT:
        exit_code = M_SIGINT;
        break;
    case SIGCHLD:
        do_job_notification();
        break;
    }
    sig_setmask(old);
}

static int ignored_signals[] = {SIGTTOU, SIGTTIN, SIGTSTP};

void initialize_signal_handling(void)
{
    if (shell_is_interactive) {
        for (size_t i = 0; i < Arr_len(ignored_signals); i++) {
            sig_ignore(ignored_signals[i]);
        }

        sig_handle(SIGINT);
    }
}

void reset_ignored_signals(void)
{
    if (shell_is_interactive) {
        for (size_t i = 0; i < Arr_len(ignored_signals); i++) {
            sig_ignore(ignored_signals[i]);
        }
    }
}
