#define _GNU_SOURCE // dprintf
#include <errno.h> // errno
#include <stdio.h> // close
#include <stdlib.h> // calloc, exit, putenv
#include <string.h> // strerror

#include <fcntl.h>
#include <sys/types.h> // pid_t
#include <sys/wait.h> // waitpid, WIF*
#include <unistd.h> // close, dup

#include "children.h" // add_bkg_child, del_bkg_proc
#include "ds/cmd.h" // cmd, cmd_wrapper
#include "ds/hash_table.h" // hash_table, add_node, find_node, free_table
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

static void fd_cleanup(int *fd_arr, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (fd_arr[i] != (int) i) {
            close(fd_arr[i]);
        }
    }
}

// Takes a cmd object and returns the output of the most recently executed
// command.
int run_cmd(cmd_wrapper const *w)
{
    int io_fd[Arr_len(w->io)] = {0, 1, 2};
    // Open IO fds
    for (size_t i = 0; i < Arr_len(w->io); i++) {
        if (w->io[i].path) {
            io_fd[i] = open(w->io[i].path, w->io[i].oflag, DEF_MODE);
        }
        Stopif(io_fd[i] == -1, fd_cleanup(io_fd, i); return M_FAILED_IO, "%s", strerror(errno));
    }

    cmd *crawler = w->root;
    crawler->fds[0] = io_fd[0];
    int ret = 0;
    while (crawler) {
        if (crawler->next) {
            int fd[2];
            pipe(fd);
            crawler->fds[1] = fd[1];
            crawler->next->fds[0] = fd[0];
        } else {
            crawler->fds[1] = io_fd[1];
            crawler->fds[2] = io_fd[2];
        }
        char **argv = crawler->argv->data;
        cmd_func f = find_node(argv[0], t);
        ret = (f) ? f(crawler) : exec_cmd(crawler);
        fd_cleanup(crawler->fds, Arr_len(io_fd));
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
    char **argv = c->argv->data;
    char **env  = c->env->data;

    if (p==0) { // Child
        for (size_t i = 0; i < c->env->num; i++) {
            Stopif(putenv(env[i]) == -1, /* No action */,
                   "Could not set the following variable/value pair: %s", env[i]);
        }

        for (size_t i = 0;  i < Arr_len(c->fds); i++){
            dup2(c->fds[i], i);
        }

        Stopif(execvp(*argv, argv) == -1, exit(M_FAILED_EXEC),"%s: %s",
               strerror(errno), *argv);
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
    // Avoid casting from void* at every use
    char **argv = c->argv->data;
    // cd to homedir if no directory specified
    char *dir = (argv[1]) ? argv[1] : getenv("HOME");
    Stopif(chdir(dir) == -1, return 1, "%s", strerror(errno));
    return 0;
}

static int m_exit(cmd const *c)
{
    (void) c;
    exit(exit_code);
}

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
    dprintf(c->fds[1], "%s\n", help_msg);
    return 0;
}
