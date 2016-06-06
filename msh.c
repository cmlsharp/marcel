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
static void parse_cmd_argv(cmd *c, char *buf);
static void gen_prompt(char *p, size_t n);
static cmd *parse_line(char *line);
static cmd *def_cmd(void);

int main(void)
{
    char p[PROMPT_LEN], buf = NULL;
    // Use tab for shell completion
    rl_bind_key('\t', rl_complete);
    if (signal(SIGINT, signal_handle) == SIG_ERR) exit(-2);
    // Ctrl_c returns control flow to here
    while (sigsetjmp(sigbuf,1) != 0);

    // Create hash table of internal commands
    if (initialize_internals() != 0) {
        fprintf(stderr, "Error initializing %s. Quitting\n", NAME);
        exit(1);
    }

    while (gen_prompt(p, PROMPT_LEN), buf = readline(p)) {
        add_history(buf);
        cmd *c = parse_line(buf);
        if (*c->argv) {
            exit_code = run_cmd(c); // MUTATING GLOBAL VARIABLE
        }
        free(buf);
        free_cmds(c);
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

// Creates linked-list of cmd objects with pipes between them
static cmd *parse_line(char *line)
{
    int fd[2];
    char *sp; // Saveptr for strtok_r
    cmd *root = def_cmd();
    if (!root) return NULL;
    cmd *crawler = root;
    char *t = strtok_r(line, "|", &sp);
    // Loop runs only if there is at least one pipe
    while (parse_cmd_argv(crawler, t), (t = strtok_r(NULL, "|", &sp))) {
        if (!*crawler->argv) return NULL;
        crawler->next = def_cmd();
        if (!crawler) return NULL;

        // Create pipe
        pipe(fd);
        crawler->out = fd[1];
        crawler = crawler->next;
        crawler->in = fd[0];
    }
    return root;
}

// Create a single cmd that takes input from stdin and outputs to stdout
static cmd *def_cmd(void)
{
    cmd *ret = calloc(1, sizeof (cmd));
    ret->out = 1;
    return ret;
}

// TODO: Implement output/input redirection
// Fills in the argv field of cmd object based on input string
static void parse_cmd_argv(cmd *c, char *buf)
{
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
