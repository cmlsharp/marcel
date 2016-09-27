#include <stdio.h> // readline
#include <stdlib.h> // calloc, getenv
#include <string.h> // strerror, strcmp

#include <unistd.h> // getcwd

#include <readline/readline.h> // readline, rl_complete
#include <readline/history.h> // add_history

#include "ds/proc.h" // proc, job etc.
#include "execute.h" // launch_job, initialize_builtins
#include "jobs.h" // initialize_job_control, do_job_notification
#include "lexer.h" // YY_BUFFER_STATE, yy_delete_buffer, yy_scan_string
#include "macros.h" // Stopif, Free
#include "parser.h" // yyparse
#include "signals.h" // initialize_signal_handling, sig_flags...

#define MAX_PROMPT_LEN 1024
int exit_code;

static inline void prepare_for_processing(void);
static inline void gen_prompt(char *buf);
static inline char *get_input(void);
static inline void add_newline(char **buf);


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
            /* Cleanup readline in order to get new prompt */           \
            rl_free_line_state();                                       \
            rl_cleanup_after_signal();                                  \
            RL_UNSETSTATE( RL_STATE_ISEARCH                             \
                         | RL_STATE_NSEARCH                             \
                         | RL_STATE_VIMOTION                            \
                         | RL_STATE_NUMERICARG                          \
                         | RL_STATE_MULTIKEY );                         \
            rl_line_buffer[rl_point = rl_end = rl_mark = 0] = 0;        \
            printf("\n");                                               \
        }                                                               \
        run_queued_signals();                                           \
        sig_flags |= WAITING_FOR_INPUT;                                 \
    } while (0)


int main(void)
{

    Stopif(!initialize_job_control(), return M_FAILED_INIT,
           "Could not initialize job control");
    Stopif(!initialize_builtins(), return M_FAILED_INIT,
           "Could not initialize builtin commands");
    initialize_signal_handling();

    // Use tab for shell completion
    rl_bind_key('\t', rl_complete);

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
        add_newline(&buf);

        YY_BUFFER_STATE b = yy_scan_string(buf);
        if ((yyparse(j) == 0) && j->root) {
            register_job(j);
            launch_job(j);
        } else {
            Cleanup(j, free_single_job);
        }

        Cleanup(b, yy_delete_buffer);
        Free(buf);

        exit_code = do_job_notification();
        prepare_for_input();
    }
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

// Grammar expects newline and readline doesn't supply it
static inline void add_newline(char **buf)
{
    size_t len = strlen(*buf);
    char *nbuf = realloc(*buf, (len + 2) * sizeof *nbuf);
    Assert_alloc(nbuf);
    nbuf[len] = '\n';
    nbuf[len+1] = '\0';
    *buf = nbuf;
}
