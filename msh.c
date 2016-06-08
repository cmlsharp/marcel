#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <signal.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "msh.h"
#include "msh_execute.h"
#include "msh.tab.h"
#include "lex.yy.h"

#define PROMPT_LEN 1024

int exit_code = 0;
sigjmp_buf sigbuf;

static void signal_handle(int signo);
static void gen_prompt(char *p, size_t n);
cmd *def_cmd(void);

int main(void)
{
    char p[PROMPT_LEN], *buf;
    // Use tab for shell completion
    rl_bind_key('\t', rl_complete);
    if (signal(SIGINT, signal_handle) == SIG_ERR) exit(-2);

    // Create hash table of internal commands
    if (initialize_internals() != 0) {
        fprintf(stderr, "Error initializing %s. Quitting\n", NAME);
        exit(1);
    }
    
    // Ctrl_c returns control flow to here
    while (sigsetjmp(sigbuf,1) != 0);
    
    while (gen_prompt(p, PROMPT_LEN), buf = readline(p)) {
        cmd *crawler = def_cmd();
        add_history(buf);
        YY_BUFFER_STATE b = yy_scan_string(buf);
        yyparse(crawler,1);
        yy_delete_buffer(b);

        if (*crawler->argv) {
            exit_code = run_cmd(crawler);
        }
        free_cmds(crawler);
    }
    cleanup_internals();
    return exit_code;
}

// Creates shell prompt based on username and current directory
static void gen_prompt(char *p, size_t n)
{
    char *user = getenv("USER");
    char *dir = getcwd(NULL, 1024);
    char sym = (strcmp(user, "root")) ? '$' : '#';
    snprintf(p, n,"%d [%s:%s] %c ", exit_code, user, dir, sym);
    free(dir);
}

// Create a single cmd that takes input from stdin and outputs to stdout
cmd *def_cmd(void)
{
    cmd *ret = calloc(1, sizeof (cmd));
    ret->out = 1;
    ret->wait = 1;
    return ret;
}

static void signal_handle(int signo)
{
    if (signo == SIGINT) {
        puts("");
        siglongjmp(sigbuf, 1); // Print new line and ignore SIGINT
    }
}
