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

#ifndef MARCEL_SIG_H
#define MARCEL_SIG_H

#include <signal.h>
#include <setjmp.h>

// This variable is only accessed via the two macros below
extern sigjmp_buf sigbuf;

extern sig_atomic_t volatile sig_flags;
enum {
    WAITING_FOR_INPUT = (1 << 0), // Process was waiting for input when it recieved signal
    QUEUE_FULL = (1 << 1),
    // To be continued...
};

void initialize_signal_handling(void);
void reset_ignored_signals(void);
void sig_default(int sig);
void sig_handle(int sig);
void run_queued_signals(void);
#endif
