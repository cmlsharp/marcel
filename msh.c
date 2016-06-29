#define _GNU_SOURCE // asprintf

#include <stdio.h> // readline
#include <stdlib.h> // _Exit, calloc, getenv
#include <string.h> // strerror, strcmp

#include <unistd.h> // getcwd

#include <readline/readline.h> // readline, rl_complete
#include <readline/history.h> // add_history

#include "msh.h" // cmd
#include "msh_execute.h" // run_cmd, initialize_internals, free_cmds
#include "msh_macros.h" // Stopif, Free
#include "msh_signals.h" // setup_signals
#include "msh.tab.h" // yyparse
#include "lex.yy.h" // YY_BUFFER_STATE, yy_delete_buffer, yy_scan_string

int volatile exit_code = 0;

static char *gen_prompt(void);
static char *get_input(void);
static void add_newline(char **buf);
cmd *def_cmd(void);


int main(void)
{
    // Use tab for shell completion
    rl_bind_key('\t', rl_complete);

    Stopif(setup_signals() != 0, return 1, "%s", strerror(errno));
    Stopif(initialize_internals() != 0, return 1, "Could not initialize internals");


    char *buf = NULL;
    while ((buf = get_input())) {
        cmd *crawler = def_cmd();
        add_history(buf);

        add_newline(&buf);
        YY_BUFFER_STATE b = yy_scan_string(buf);
        if ((yyparse(crawler, 1) == 0) && *crawler->argv) {
            exit_code = run_cmd(crawler); // Mutate global variable
        }

        // Cleanup
        yy_delete_buffer(b);
        free_cmds(crawler);
        Free(buf);
    }
    return exit_code;
}

// Prints prompt and returns line entered by user. Returned string must be
// freed. Returns NULL on EOF
static char *get_input(void)
{
    char *p = gen_prompt();
    char *line = readline(p);
    Free(p);
    return line;
}

// Creates shell prompt based on username and current directory
static char *gen_prompt(void)
{
    char *p = NULL;
    char *user = getenv("USER");
    char *dir = getcwd(NULL, 1024);
    char sym = (strcmp(user, "root")) ? '$' : '#';
    Stopif(asprintf(&p, "%d [%s:%s] %c ", exit_code, user, dir, sym) == -1,
           _Exit(2), "Memory allocation error. Quiting");
    Free(dir);
    return p;
}

// Create a single cmd that takes input from stdin and outputs to stdout
cmd *def_cmd(void)
{
    cmd *ret = calloc(1, sizeof (cmd));
    Stopif(!ret, _Exit(2), "Memory allocation error. Quiting.");
    ret->out = 1;
    ret->wait = 1;
    return ret;
}


// Grammar expects newline and readline doesn't supply it
static void add_newline(char **buf)
{
    char *nbuf;
    Stopif(asprintf(&nbuf, "%s\n", *buf) == -1, _Exit(2),
           "Memory allocation error. Quitting");
    Free(*buf);
    *buf = nbuf;
}

