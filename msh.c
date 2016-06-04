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

#define PARSE_DELIM " \t\n\r"
#define PROMPT_LEN 1024

int exit_code = 0; 
sigjmp_buf sigbuf;

static void signal_handle(int signo);
static void parse_cmd(cmd *c, char *buf);
static void gen_prompt(char *p, size_t n);

int main(void)
{
    char p[PROMPT_LEN], *buf = NULL;
    rl_bind_key('\t', rl_complete);
    
    if (signal(SIGINT, signal_handle) == SIG_ERR) exit(-2);
    while (sigsetjmp(sigbuf,1) != 0);

    if (initialize_internals() != 0) {
        fprintf(stderr, "Error initializing %s. Quitting\n", NAME);
        exit(1);
    }

    while (gen_prompt(p, PROMPT_LEN), buf = readline(p)) {
        cmd c = CMD_DEF;
        parse_cmd(&c, buf);
        if (*c.argv) {
            exit_code = run_cmd(&c); // MUTATING GLOBAL VARIABLE
            add_history(buf);
        }
        free(buf);
    }
    cleanup_internals();
    return exit_code;
}

static void gen_prompt(char *p, size_t n)
{
    char *user = getenv("USER");
    char *dir = getcwd(NULL, 1024);
    char sym = (strcmp(user, "root")) ? '$' : '#';
    snprintf(p, n,"%d [%s:%s] %c ", exit_code, user, dir, sym);
    free(dir);
}

//TODO: Implement output/input redirection
static void parse_cmd(cmd *c, char *buf)
{
    if (!c->argv) {
        return;
    }
    char *t = strtok(buf, PARSE_DELIM);
    for (size_t i = 0; t && i < MAX_ARGS; i++) {
        c->argv[i] = t;
        t = strtok(NULL, PARSE_DELIM);
    }
}

static void signal_handle(int signo)
{
    if (signo == SIGINT) {
        puts("");
        siglongjmp(sigbuf, 1); // Print new line and ignore SIGINT
    } 
}
