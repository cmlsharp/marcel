#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "msh_execute.h"
#include "hash_table.h"

#define NUM_BUILTINS (sizeof builtin_names / sizeof (char*))

static int msh_cd(cmd const *c);
static int exec_cmd(cmd const *c);
static int msh_exit(cmd const *c);

// Names of shell builtins
char const *builtin_names[] = {
    "cd",
    "exit"
};

// Functions associated with shell builtins
cmd_func const builtin_funcs[] = {
    &msh_cd,
    &msh_exit
};

// Hash table for shell builtins
node *table[TABLE_SIZE] = {0};

// Create hashtable of shell builtins
int initialize_internals(void)
{
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        if (add_node(builtin_funcs[i], builtin_names[i], table) != 0)
            return -1;
    }
    return 0;
}

// Wrapper around free_table function so that table need not be visible to
// caller
void cleanup_internals(void)
{
    free_table(table);
}

// Free list of command objects
void free_cmds(cmd *crawler)
{
    while (crawler) {
        for (size_t i = 0; crawler->argv[i] && i < MAX_ARGS; i++) free(crawler->argv[i]);
        cmd *next = crawler->next;
        free(crawler);
        crawler = next;
    }
}

// Takes a cmd object and returns the output of the most recently executed
// command.
int run_cmd(cmd *const c)
{
    cmd *crawler = c;
    int ret = 0;
    while (crawler) {
        // Check if cmd is builtin
        cmd_func f = find_node(*crawler->argv, table);
        ret = (f) ? f(crawler) : exec_cmd(crawler);
        if (crawler->in != 0) close(crawler->in);
        if (crawler->out != 1) close(crawler->out);
        crawler = crawler->next;
    }
    return ret;
}

static int exec_cmd(cmd const *c)
{
    int status;
    pid_t p = fork();

    if (p==0) { // Child
        dup2(c->in, STDIN_FILENO);
        dup2(c->out, STDOUT_FILENO);
        if (execvp(*c->argv, c->argv) == -1) {
            perror(NAME);
            cleanup_internals();
            free_cmds((cmd*) c);
            exit(-1);
        }
    } else if (p>0) {
        if (!c->wait) return 0;
        do {
            waitpid(p, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        return WEXITSTATUS(status); // Return exit code
    } else
        perror(NAME);
    return 1;
}

static int msh_cd(cmd const *c)
{
    // cd to homedir if no directory specified
    char *dir = (c->argv[1]) ? c->argv[1] : getenv("HOME");
    if (chdir(dir) == -1) {
        perror(NAME);
        return 1;
    }
    return 0;
}

static int msh_exit(cmd const *c)
{
    free_cmds((cmd*)c);
    cleanup_internals();
    exit(exit_code);
}
