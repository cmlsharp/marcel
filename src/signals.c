/*
 * Marcel the Shell -- a shell written in C
 * Copyright (C) 2016 Chad Sharp
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// NOTE: Much of the below was inspired by the signal handling found in Z Shell
// (particularly the helper functions and signal queue)

#include <stdio.h>
#include <stdint.h>

#include <signal.h>

#include "jobs.h"
#include "macros.h"
#include "signals.h"

// SIG_ATOMIC_MAX is only guaranteed to be at least 127
#if ((SIG_ATOMIC_MAX - 1) < 1024)
#define MAX_QUEUE_SIZE ((size_t) SIG_ATOMIC_MAX)
#else
#define MAX_QUEUE_SIZE ((size_t) 1024)
#endif

#ifndef __GNUC__ 
#define __attribute__(X)
#endif


typedef struct sigstate {
    int sig;
    sigset_t mask;
    sig_atomic_t flags;
} sigstate;

static void handler_sync(sigstate state);
static void handler_async(int signo);

// Buffer to allow jumps out of signal handler
sigjmp_buf sigbuf;

sig_atomic_t volatile queue_front, queue_back;
sigstate volatile signal_queue[MAX_QUEUE_SIZE];

// Bitmask passed to signal handler
sig_atomic_t volatile sig_flags;

__attribute__((always_inline))
extern inline void sig_handle(int sig)
{
    struct sigaction act = {0};
    sigemptyset(&act.sa_mask);
    act.sa_handler = handler_async;
    sigaction(sig, &act, NULL);
}


__attribute__((always_inline))
static inline sigset_t sig_block(sigset_t new)
{
    sigset_t old;
    // Just in case sigprocmask fails
    sigemptyset(&old);
    sigprocmask(SIG_BLOCK, &new, &old);
    return old;
}

// Remove signals in `new` from signal mask and return previous signal mask
__attribute__((always_inline))
extern inline sigset_t sig_unblock(sigset_t new)
{
    sigset_t old;
    // Just in case sigprocmask fails
    sigemptyset(&old);
    sigprocmask(SIG_UNBLOCK, &new, &old);
    return old;
}

// Set signal mask, return previous signal mask
__attribute__((always_inline))
static inline sigset_t sig_setmask(sigset_t new)
{
    sigset_t old;
    // Just in case sigprocmask fails
    sigemptyset(&old);
    sigprocmask(SIG_SETMASK, &new, &old);
    return old;
}

// Ignore signal
__attribute__((always_inline))
extern inline void sig_ignore(int sig)
{
    signal(sig, SIG_IGN);
}

// Reset signal handler to default action
__attribute__((always_inline))
extern inline void sig_default(int sig)
{
    signal(sig, SIG_DFL);
}

// Signal queueing
__attribute__((always_inline))
extern inline void run_queued_signals(void)
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
    if (temp != queue_front) {
        queue_back = temp;
        signal_queue[queue_back] = (sigstate) {
            .sig = signo, .mask = old, .flags = sig_flags,
        };
    } else {
        // This really shouldn't ever happen
        sig_flags |= QUEUE_FULL;
    }

    if (sig_flags & WAITING_FOR_INPUT) {
        siglongjmp(sigbuf, 1);
    }
    // Process automatically restores mask when handler returns
    // sig_setmask(old);

}

static void handler_sync(sigstate state)
{
    sigset_t old = sig_setmask(state.mask);
    Stopif(sig_flags & QUEUE_FULL, sig_flags &= ~QUEUE_FULL,
           "At least one signal was not handled because signal queue was full");
    switch (state.sig) {
    case SIGINT:
        exit_code = M_SIGINT;
        break;
    case SIGCHLD:
        exit_code = do_job_notification();
        break;
    }
    sig_setmask(old);
}

static int ignored_signals[] = {SIGTTOU, SIGTTIN, SIGTSTP};

void initialize_signal_handling(void)
{
    if (interactive) {
        for (size_t i = 0; i < Arr_len(ignored_signals); i++) {
            sig_ignore(ignored_signals[i]);
        }

        sig_handle(SIGINT);
    }
}

void reset_ignored_signals(void)
{
    if (interactive) {
        for (size_t i = 0; i < Arr_len(ignored_signals); i++) {
            sig_ignore(ignored_signals[i]);
        }
    }
}
