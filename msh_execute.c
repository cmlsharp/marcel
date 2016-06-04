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

//TODO: Fix this hacky garbage w/ apropriate data structure (hash table)

char const *builtin_names[] = {
    "cd",
    "exit"
};

cmd_func const builtin_funcs[] = {
    &msh_cd,
    &msh_exit
};

node *table[TABLE_SIZE] = {0};

int initialize_internals(void) {
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        if (add_node(builtin_funcs[i], builtin_names[i], table) != 0)
            return -1;
    }
    return 0;
}

void cleanup_internals(void) {
    free_table(table);
}


int run_cmd(cmd *const c)
{
    cmd_func f = find_node(*c->argv, table);
    return (f) ? f(c) : exec_cmd(c);
}

static int exec_cmd(cmd const *c)
{
    int status;
    pid_t p = fork();

    if (p==0) {
        if (execvp(*c->argv, c->argv) == -1) {
            perror(NAME);
            exit(-1);
        }
    } else if (p>0) {
        do {
            waitpid(p, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        return WEXITSTATUS(status);
    } else {
        perror(NAME);
    }
    return 1;
}

static int msh_cd(cmd const *c)
{
    char *dir = (c->argv[1]) ? c->argv[1] : getenv("HOME");
    if (chdir(dir) == -1) {
        perror(NAME);
        return 1;
    }
    return 0;
}

static int msh_exit(cmd const *c)
{
    exit(exit_code);
    return 0; // Never reached
}
