#ifndef __cplusplus
#define _GNU_SOURCE // asprintf
#endif

#include <stdio.h> // readline
#include <stdlib.h> // calloc, getenv
#include <string.h> // strerror, strcmp

#include <unistd.h> // getcwd

#include <readline/readline.h> // readline, rl_complete
#include <readline/history.h> // add_history

#include "ds/cmd.h" // cmd, cmd_wrapper etc.
#include "execute.h" // run_cmd, initialize_internals
#include "macros.h" // Stopif, Free
#include "signals.h" // setup_signals
#include "parser.h" // yyparse
#include "lexer.h" // YY_BUFFER_STATE, yy_delete_buffer, yy_scan_string

#define MAX_PROMPT_LEN 1024
int volatile exit_code = 0;

static char *get_input(void);
static void gen_prompt(char *buf);
static void add_newline(char **buf);

int main(void)
{
    // Use tab for shell completion
    rl_bind_key('\t', rl_complete);
    rl_clear_signals();
    Stopif(setup_signals() != 0, return M_FAILED_INIT, "%s", strerror(errno));
    Stopif(initialize_internals() != 0, return M_FAILED_INIT, "Could not initialize internals");

    // buf, b and wrap must be volatile becasue they are read from/written
    // to between Sigint_reentry (which calls sigsetbuf) and when siglongjmp
    // could be called in the SIGINT handler. Suboptimal but seemingly
    // necessary.
    char *volatile buf = NULL;
    // SIGINT before first command is executed returns here
    Sigint_init_reentry();
    while ((buf = get_input())) {
        cmd_wrapper *volatile wrap = new_cmd_wrapper();
        add_history(buf);
        add_newline((char **) &buf);
        YY_BUFFER_STATE volatile b = yy_scan_string(buf);
        if ((yyparse(wrap) == 0) && wrap->root) {
            exit_code = run_cmd(wrap); // Mutate global variable
        }

        // Cleanup
        Sigint_reentry();

        // Free and set to NULL to ensure SIGINT in next loop iteration doesn't
        // result in double free
        Cleanup(b, yy_delete_buffer);
        Cleanup(wrap, free_cmd_wrapper);
        Free(buf);
    }
    return exit_code;
}

// Prints prompt and returns line entered by user. Returned string must be
// freed. Returns NULL on EOF
static char *get_input(void)
{
    char prompt_buf[MAX_PROMPT_LEN] = {0};
    gen_prompt(prompt_buf);
    char *line = readline(prompt_buf);
    return line;
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
    char *nbuf;
    Assert_alloc(asprintf(&nbuf, "%s\n", *buf) != -1);
    Free(*buf);
    *buf = nbuf;
}


