#define _GNU_SOURCE // dprintf
#include <errno.h> // errno
#include <stdio.h> // close
#include <stdlib.h> // calloc, exit, putenv
#include <string.h> // strerror

#include <sys/types.h> // pid_t
#include <sys/wait.h> // waitpid, WIF*
#include <unistd.h> // close, dup

#include "marcel.h" // free_cmd
#include "hash_table.h" // hash_table, add_node, find_node, free_table
#include "children.h" // add_bkg_child, del_bkg_proc
#include "execute.h" // cmd_func
#include "macros.h" // Stopif, Free, Arr_len

static int exec_cmd(cmd const *c);
static void cleanup_internals(void);
static int m_cd(cmd const *c);
static int m_exit(cmd const *c);
static int m_help(cmd const *c);

// Names of shell builtins
char const *builtin_names[] = {
    "cd",
    "exit",
    "help",
};

// Functions associated with shell builtins
cmd_func const builtin_funcs[] = {
    &m_cd,
    &m_exit,
    &m_help
};

// Hash table for shell builtins
hash_table *t;

// Create hashtable of shell builtins
int initialize_internals(void)
{
    t = new_table(TABLE_INIT_SIZE);
    // NOTE: We are mixing data pointers and function pointers here. ISO C
    // forbids this but it's fine in POSIX
    for (size_t i = 0; i < Arr_len(builtin_names); i++) {
        if (add_node(builtin_names[i], builtin_funcs[i], t) != 0) {
            return -1;
        }
    }

    if (atexit(cleanup_internals) != 0) {
        return -1;
    }
    return 0;
}

// Wrapper around free_table so it can be passed to atexit
void cleanup_internals(void)
{
    free_table(&t);
}

// Takes a cmd object and returns the output of the most recently executed
// command.
int run_cmd(cmd const *c)
{
    cmd const *crawler = c;
    int ret = 0;
    while (crawler) {
        // Check if cmd is builtin
        // Mixing data/function pointers again
        cmd_func f = find_node(*crawler->argv.strs, t);
        ret = (f) ? f(crawler) : exec_cmd(crawler);
        if (crawler->in != 0) {
            close(crawler->in);
        }
        if (crawler->out != 1) {
            close(crawler->out);
        }
        if (crawler->err != 2) {
            close(crawler->err);
        }
        crawler = crawler->next;
    }
    return ret;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
static int exec_cmd(cmd const *c)
{
    int status = 0;
    pid_t p = fork();

    Stopif(p < 0, return 1, "%s", strerror(errno));

    if (p==0) { // Child
        dup2(c->in , 0);
        dup2(c->out, 1);
        dup2(c->err, 2);
        for (size_t i = 0; i < c->env.num; i++) {
            Stopif(putenv(c->env.strs[i]) == -1, /* No action */,
                   "Could not set the following variable/value pair: %s", c->env.strs[i]);
        }
        Stopif(execvp(*c->argv.strs, c->argv.strs) == -1, exit(M_FAILED_EXEC),"%s: %s",
               strerror(errno), *c->argv.strs);
    } else if (p>0) {
        if (!c->wait) {
            size_t job_num = add_bkg_child(p);
            Stopif(job_num == 0, return 1, "Maximum number of background jobs reached");
            printf("Background job number [%zu] created\n", job_num);
            return 0;
        }
        set_active_child(p);
        do {
            waitpid(p, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));
        return WEXITSTATUS(status); // Return exit code
    }
}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

static int m_cd(cmd const *c)
{
    // cd to homedir if no directory specified
    char *dir = (c->argv.strs[1]) ? c->argv.strs[1] : getenv("HOME");
    Stopif(chdir(dir) == -1, return 1, "%s", strerror(errno));
    return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static int m_exit(cmd const *c)
{
    exit(exit_code);
}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

static int m_help(cmd const *c)
{
    char *help_msg = "Marcel the Shell (with shoes on) v. " VERSION "\n"
                     "Written by Chad Sharp\n"
                     "\n"
                     "Features:\n"
                     "* IO redirection (via < > and >>)\n"
                     "* Pipes\n"
                     "* Command history\n"
                     "* Background jobs (via &)\n"
                     "\n"
                     "This shell only fights when provoked.";
    dprintf(c->out, "%s\n", help_msg);
    return 0;
}
