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

#include <stdio.h> // readline
#include <stdlib.h> // calloc, getenv
#include <string.h> // strerror, strcmp

#include <unistd.h> // getcwd

#include <readline/readline.h> // readline, rl_complete
#include <readline/history.h> // add_history

#include "signals.h" // initialize_signal_handling, sig_flags...
#include "ds/proc.h" // proc, job etc.
#include "execute.h" // launch_job, initialize_builtins
#include "jobs.h" // initialize_job_control, report_job_status
#include "lexer.h" // YY_BUFFER_STATE, yy_delete_buffer, yy_scan_string
#include "macros.h" // Stopif, Free
#include "parser.h" // yyparse

#define MAX_PROMPT_LEN 1024
#define HIST_FILE ".marcel.hist"
int exit_code;

static char *saved_line;
static int saved_point;
static inline int restore_buffer(void);
static inline void prepare_for_processing(void);
static inline void gen_prompt(char *buf);
static inline char *path_concat(char *dir, char *file);
static inline char *get_input(void);

// This has to ba a macro because sigsetjmp is picky about the its stack frame
// it returns into
// Turns on asynchronous handling for SIGCHLD and is the destination for the
// sigsetjmp calls in the handler. When returning from longjmp, ensures
// readline's temporary variables are cleaned up. Additionally, informs signal
// handler that it should longjmp out
#define prepare_for_input()                                             \
    do {                                                                \
        sig_handle(SIGCHLD);                                            \
        /* siglongjmp from signal handler returns here */               \
        while (sigsetjmp(sigbuf, 1)) {                                  \
            sig_flags &= ~WAITING_FOR_INPUT;                            \
            if (!(sig_flags & NO_RESTORE)) {                            \
                saved_line  = rl_copy_text(0, rl_end);                  \
                saved_point = rl_point;                                 \
                rl_pre_input_hook = restore_buffer;                     \
                rl_replace_line("",0);                                  \
                rl_redisplay();                                         \
            }                                                           \
            putchar('\n');                                              \
            sig_flags &= ~NO_RESTORE;                                   \
        }                                                               \
        run_queued_signals();                                           \
        sig_flags |= WAITING_FOR_INPUT;                                 \
    } while (0)

int main(void)
{

    Stopif(!initialize_builtins(), return M_FAILED_INIT,
           "Could not initialize builtin commands");
    Stopif(!initialize_job_control(), return M_FAILED_INIT,
           "Could not initialize job control");
    initialize_signal_handling();

    // Use tab for shell completion
    rl_bind_key('\t', rl_complete);
    rl_set_signals();

    // Setup history
    char *home = getenv("HOME");
    char *hist_path = path_concat(home, HIST_FILE);
    read_history(hist_path);

    // buffer for stdin
    char *buf = NULL;

    prepare_for_input();
    while ((buf = get_input())) {
        prepare_for_processing();

        job *j = new_job();
        j->name = malloc((strlen(buf) + 1) * sizeof *j->name);
        Assert_alloc(j->name);

        strcpy(j->name, buf);
        add_history(buf);

        YY_BUFFER_STATE b = yy_scan_string(buf);
        if (!yyparse(j) && j->valid) {
            register_job(j);
            launch_job(j);
        } else {
            Cleanup(j, free_single_job);
        }

        Cleanup(b, yy_delete_buffer);
        Free(buf);

        exit_code = report_job_status();
        prepare_for_input();
    }
    write_history(hist_path);
    free(hist_path);
    return exit_code;
}


// For symmetry with prepare_for_input. Turns off async handling for SIGCHLD
// and informs signal handler that it shouldn't longjmp out
static inline void prepare_for_processing(void)
{
    sig_flags &= ~WAITING_FOR_INPUT;
    sig_default(SIGCHLD);
}


// Prints prompt and returns line entered by user. Returned string must be
// freed. Returns NULL on EOF
static inline char *get_input(void)
{
    char prompt_buf[MAX_PROMPT_LEN] = {0};
    gen_prompt(prompt_buf);
    return readline(prompt_buf);
}

// Creates shell prompt based on username and current directory
static inline void gen_prompt(char *buf)
{
    char *user = getenv("USER");
    char *dir = getcwd(NULL, 1024);
    char sym = (strcmp(user, "root")) ? '$' : '#';
    snprintf(buf, MAX_PROMPT_LEN, "%-3d [%s:%s] %c ", (unsigned char) exit_code,
             user, dir, sym);
    Free(dir);
}

// Restores the user's input to readline's buffer
static inline int restore_buffer(void)
{
    // Temporarily block signals;
    sigset_t new;
    sigfillset(&new);
    sigset_t old = sig_block(new);
    rl_replace_line(saved_line, 0);
    rl_point = saved_point;
    rl_redisplay();
    saved_point = 0;
    Free(saved_line);
    rl_pre_input_hook = NULL;
    // Restore signal mask
    sig_setmask(old);
    return 0;
}

static inline char *path_concat(char *dir, char *file) 
{
    size_t dlen = strlen(dir);
    size_t len = dlen + strlen(file) + 2;
    char *buf = malloc(len * sizeof *buf);
    Assert_alloc(buf);
    strcpy(buf, dir);
    buf[dlen] = '/';
    strcpy(buf + dlen + 1, file);
    return buf;
}
