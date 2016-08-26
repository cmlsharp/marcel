#ifndef MARCEL_SIG_H
#define MARCEL_SIG_H

#include <signal.h>
#include <setjmp.h>

// This variable is only accessed via the two macros below
extern sigjmp_buf _sigbuf;

extern sig_atomic_t sig_flags;
enum {
    WAITING_FOR_INPUT = (1 << 0) // Process was waiting for input when it recieved signal
                        // To be continued...
};

void initialize_signal_handling(void);
void reset_ignored_signals(void);
void sig_default(int sig);
void sig_handle(int sig);
void run_queued_signals(void);
#endif
