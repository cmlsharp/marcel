#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "msh_execute.h"

#define NUM_BUILTINS (sizeof builtin_names / sizeof (char*))

typedef int (*cmd_func)(cmd const*);

static int msh_cd(cmd const *c);
static int exec_cmd(cmd const *c);
static int msh_exit(cmd const *c);

//TODO: Fix this hacky garbage w/ apropriate data structure (hash table)

char const * const builtin_names[] = {
    "cd",
    "exit"
};

cmd_func const builtin_funcs[] = {
    &msh_cd,
    &msh_exit
};


int run_cmd(cmd *const c)
{
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(builtin_names[i], c->argv[0]) == 0) {
            return (builtin_funcs[i])(c);
        }
    }
    return exec_cmd(c);
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
