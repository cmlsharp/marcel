#define _GNU_SOURCE // asprintf

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>

#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "msh.h"
#include "msh_internals.h"
#include "msh_macros.h"
#include "msh.tab.h"
#include "lex.yy.h"

int exit_code = 0;
sigjmp_buf sigbuf;

static void signal_handle(int signo);
static char *gen_prompt(void);
static void add_newline(char **buf);
static int setup_signals(void);
cmd *def_cmd(void);


int main(void)
{
    // Use tab for shell completion
    rl_bind_key('\t', rl_complete);

    Stopif(setup_signals() != 0, return 1, strerror(errno));
    Stopif(initialize_internals() != 0, return 1, "Could not initialize internals");

    // Ctrl_c returns control flow to here
    while (sigsetjmp(sigbuf,1) != 0);

    char *buf, *p;
    while ((p = gen_prompt()), buf = readline(p)) {
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
        Free_all(buf, p);
    }

    Free(p);
    return exit_code;
}

// Creates shell prompt based on username and current directory
static char *gen_prompt(void)
{
    char *p = NULL;
    char *user = getenv("USER");
    char *dir = getcwd(NULL, 1024);
    char sym = (strcmp(user, "root")) ? '$' : '#';
    Stopif(asprintf(&p, "%d [%s:%s] %c ", exit_code, user, dir, sym) == -1, _Exit(2), "Memory allocation error. Quiting");
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

static void signal_handle(int signo)
{
    switch (signo) {
    case SIGINT:
        if (signo == SIGINT) {
            puts("");
            siglongjmp(sigbuf, 1); // Print new line and ignore SIGINT
        }
        break;
    case SIGCHLD:
        waitpid(-1, NULL , WNOHANG | WUNTRACED);
        break;
    }
}

// Grammar expects newline and readline doesn't supply it
static void add_newline(char **buf)
{
    char *nbuf;
    Stopif(asprintf(&nbuf, "%s\n", *buf) == -1, _Exit(2), "Memory allocation error. Quitting");
    Free(*buf);
    *buf = nbuf;
}

static int setup_signals(void)
{
    struct sigaction act = {0};
    act.sa_handler = signal_handle;
    if (sigemptyset(&act.sa_mask) == -1) {
        return 1;
    }
    return ((sigaction(SIGINT, &act, NULL) == -1) ||
            (sigaction(SIGCHLD, &act, NULL)  == -1));
}
