#define _GNU_SOURCE // asprintf

#include <stdio.h> // readline
#include <stdlib.h> // calloc, getenv
#include <string.h> // strerror, strcmp

#include <unistd.h> // getcwd

#include <readline/readline.h> // readline, rl_complete
#include <readline/history.h> // add_history

#include "ds/cmd.h" // cmd, job etc.
#include "execute.h" // launch_job, initialize_builtins
#include "jobs.h" // initialize_job_control, do_job_notification
#include "lexer.h" // YY_BUFFER_STATE, yy_delete_buffer, yy_scan_string
#include "macros.h" // Stopif, Free
#include "parser.h" // yyparse
#include "signals.h" // setup_signals

#define MAX_PROMPT_LEN 1024
int exit_code = 0;

static void cleanup_readline(void);
static void gen_prompt(char *buf);
static char *get_input(void);
static void add_newline(char **buf);

int main(void)
{

    Stopif(!initialize_job_control(), return M_FAILED_INIT, "Could not initialize job control");
    Stopif(!initialize_builtins(), return M_FAILED_INIT, "Could not initialize builtin commands");
    initialize_signal_handling();

    rl_clear_signals();

    // Use tab for shell completion
    rl_bind_key('\t', rl_complete);

    // buffer for stdin
    char *buf = NULL;

    // SIGINT before first command returns here
    while (sigsetjmp(_sigbuf, 1)) cleanup_readline();
    run_queued_signals();
    sig_flags |= WAITING_FOR_INPUT;
    
    while ((buf = get_input())) {
        sig_flags &= ~WAITING_FOR_INPUT;
        sig_default(SIGCHLD);

        job *j = new_job();
        j->name = strdup(buf);
        Assert_alloc(j->name);

        add_history(buf);
        add_newline((char **) &buf);

        YY_BUFFER_STATE b = yy_scan_string(buf);
        if ((yyparse(j) == 0) && j->root) {
            register_job(j);
            launch_job(j); 
        } else {
            Cleanup(j, free_single_job);
        }

        do_job_notification();
        sig_handle(SIGCHLD);

        Cleanup(b, yy_delete_buffer);
        Free(buf);


        // If returning from siglongjmp, cleanup readline and give new prompt
        while (sigsetjmp(_sigbuf, 1)) cleanup_readline();
        run_queued_signals();
        sig_flags |= WAITING_FOR_INPUT;

    }
    return exit_code;
}

// Cleanup readline in order to get a new prompt
static void cleanup_readline(void) 
{
    rl_free_line_state();
    rl_cleanup_after_signal();
    RL_UNSETSTATE(RL_STATE_ISEARCH|RL_STATE_NSEARCH|RL_STATE_VIMOTION|RL_STATE_NUMERICARG|RL_STATE_MULTIKEY);
    rl_line_buffer[rl_point = rl_end = rl_mark = 0] = 0;
    printf("\n");
}

// Prints prompt and returns line entered by user. Returned string must be
// freed. Returns NULL on EOF
static char *get_input(void)
{
    char prompt_buf[MAX_PROMPT_LEN] = {0};
    gen_prompt(prompt_buf);
    return readline(prompt_buf);
}

// Creates shell prompt based on username and current directory
static void gen_prompt(char *buf)
{
    char *user = getenv("USER");
    char *dir = getcwd(NULL, 1024);
    char sym = (strcmp(user, "root")) ? '$' : '#';
    snprintf(buf, MAX_PROMPT_LEN, "%-3d [%s:%s] %c ", (unsigned char) exit_code,
             user, dir, sym);
    Free(dir);
}

// Grammar expects newline and readline doesn't supply it
static void add_newline(char **buf)
{
    char *nbuf;
    Assert_alloc(asprintf(&nbuf, "%s\n", *buf) != -1);
    Free(*buf);
    *buf = nbuf;
}

