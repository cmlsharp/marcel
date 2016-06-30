#define _GNU_SOURCE // asprintf

#include <stdio.h> // readline
#include <stdlib.h> // _Exit, calloc, getenv
#include <string.h> // strerror, strcmp

#include <unistd.h> // getcwd

#include <readline/readline.h> // readline, rl_complete
#include <readline/history.h> // add_history

#include "marcel.h" // cmd
#include "execute.h" // run_cmd, initialize_internals, free_cmds
#include "macros.h" // Stopif, Free
#include "signals.h" // setup_signals
#include "marcel.tab.h" // yyparse
#include "lex.yy.h" // YY_BUFFER_STATE, yy_delete_buffer, yy_scan_string

#define MAX_PROMPT_LEN 1024
int volatile exit_code = 0;

static void gen_prompt(char *buf);
static char *get_input(void);
static void add_newline(char **buf);
cmd *def_cmd(void);


int main(void)
{
    // Use tab for shell completion
    rl_bind_key('\t', rl_complete);

    rl_clear_signals();
    Stopif(setup_signals() != 0, return 1, "%s", strerror(errno));
    Stopif(initialize_internals() != 0, return 1, "Could not initialize internals");

    // buf, b and crawler must be volatile becasue they are read from/written
    // to between Sigint_reentry (which calls sigsetbuf) and when siglongjmp 
    // could be called in the SIGINT handler. Suboptimal but seemingly
    // necessary.
    char *volatile buf = NULL;
    // SIGINT before first command is executed returns here
    Sigint_init_reentry();
    while ((buf = get_input())) {
        cmd *volatile crawler = def_cmd();
        add_history(buf);
        add_newline((char **) &buf);
        YY_BUFFER_STATE volatile b = yy_scan_string(buf);
        if ((yyparse(crawler, 1) == 0) && *crawler->argv) {
            exit_code = run_cmd(crawler); // Mutate global variable
        }

        // Cleanup
        Sigint_reentry();
        
        // Free and set to NULL to ensure SIGINT in next loop iteration doesn't
        // result in double free
        Cleanup(b, yy_delete_buffer);
        Cleanup(crawler, free_cmds);
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
    snprintf(buf, MAX_PROMPT_LEN, "%-3d [%s:%s] %c ", (unsigned char) exit_code, user, dir, sym);
    Free(dir);
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

