#define _GNU_SOURCE // asprintf

#include <stdio.h> // readline
#include <stdlib.h> // calloc, getenv
#include <string.h> // strerror, strcmp

#include <unistd.h> // getcwd

#include <readline/readline.h> // readline, rl_complete
#include <readline/history.h> // add_history

#include "marcel.h" // cmd
#include "execute.h" // run_cmd, initialize_internals
#include "macros.h" // Stopif, Free
#include "signals.h" // setup_signals
#include "parser.h" // yyparse
#include "lexer.h" // YY_BUFFER_STATE, yy_delete_buffer, yy_scan_string

#define MAX_PROMPT_LEN 1024
int volatile exit_code = 0;

cmd *new_cmd(void);
void free_cmd(cmd *c);

static cmd_wrapper *new_cmd_wrapper(void);
static void free_cmd_wrapper(cmd_wrapper *w);
static void gen_prompt(char *buf);
static char *get_input(void);
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
        if ((yyparse(wrap) == 0) && *wrap->root->argv.strs) {
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
static void gen_prompt(char *buf)
{
    char *user = getenv("USER");
    char *dir = getcwd(NULL, 1024);
    char sym = (strcmp(user, "root")) ? '$' : '#';
    snprintf(buf, MAX_PROMPT_LEN, "%-3d [%s:%s] %c ", (unsigned char) exit_code,
             user, dir, sym);
    Free(dir);
}

static cmd_wrapper *new_cmd_wrapper(void)
{
    cmd_wrapper *ret = calloc(1, sizeof *ret);
    Assert_alloc(ret);
    return ret;
}

// Create a single cmd that takes input from stdin and outputs to stdout
cmd *new_cmd(void)
{
    cmd *ret = calloc(1, sizeof *ret);
    Assert_alloc(ret);

    Foreach(str_array *, x, &ret->argv, &ret->env) {
        (*x)->strs = calloc(ARGV_INIT_SIZE, sizeof (char*));
        Assert_alloc((*x)->strs);
        (*x)->cap = ARGV_INIT_SIZE;
    }

    for (size_t i = 0; i < Arr_len(ret->fds); i++) {
        ret->fds[i] = i;
    }

    ret->wait = 1;
    return ret;
}

// Free list of command objects
void free_cmd(cmd *c)
{
    while (c) {
        Foreach(str_array *, x, &c->argv, &c->env) {
            for (size_t i = 0; (*x)->strs[i] && i < (*x)->cap; i++) {
                Free((*x)->strs[i]);
            }
            Free((*x)->strs);
        }

        cmd *next = c->next;
        Free(c);
        c = next;
    }
}

static void free_cmd_wrapper(cmd_wrapper *w)
{
    if (!w) return;
    for (size_t i = 0; i < Arr_len(w->io); i++) {
        Free(w->io[i].path);
    }
    free_cmd(w->root);
    Free(w);
}


// Grammar expects newline and readline doesn't supply it
static void add_newline(char **buf)
{
    char *nbuf;
    Assert_alloc(asprintf(&nbuf, "%s\n", *buf) != -1);
    Free(*buf);
    *buf = nbuf;
}

