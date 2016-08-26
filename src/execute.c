#define _GNU_SOURCE // dprintf
#include <errno.h> // errno
#include <stdio.h> // close
#include <stdlib.h> // calloc, exit, putenv
#include <string.h> // strerror

#include <fcntl.h> // open, close
#include <sys/types.h> // pid_t
#include <unistd.h> // close, dup, getpid, setpgid, tcsetpgrp

#include "ds/cmd.h" // cmd, job
#include "ds/hash_table.h" // hash_table, add_node, find_node, free_table
#include "execute.h" // cmd_func
#include "jobs.h" // shell_is_interactive, shell_term, wait_for_job, put_job_in_*...
#include "macros.h" // Stopif, Free, Arr_len
#include "signals.h" // reset_signals

static void cleanup_builtins(void);
static void exec_cmd(cmd const *c);
static int m_cd(cmd const *c);
static int m_exit(cmd const *c);
static int m_help(cmd const *c);

// Names of shell builtins
static char const *builtin_names[] = {
    "cd",
    "exit",
    "help",
};

// Functions associated with shell builtins
static cmd_func const builtin_funcs[] = {
    &m_cd,
    &m_exit,
    &m_help
};

// Hash table for shell builtins
static hash_table *t;

// Create hashtable of shell builtins
// Returns true on success, false on failure
_Bool initialize_builtins(void)
{
    t = new_table(TABLE_INIT_SIZE);
    // NOTE: We are mixing data pointers and function pointers here. ISO C
    // forbids this but it's fine in POSIX
    for (size_t i = 0; i < Arr_len(builtin_names); i++) {
        if (add_node(builtin_names[i], builtin_funcs[i], t) != 0) {
            return 0;
        }
    }

    if (atexit(cleanup_builtins) != 0) {
        return 0;
    }
    return 1;
}

// Wrapper around free_table so it can be passed to atexit
static void cleanup_builtins(void)
{
    Cleanup(t, free_table);
}

static void fd_cleanup(int *fd_arr, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (fd_arr[i] != (int) i) {
            close(fd_arr[i]);
        }
    }
}

// Set process group for process and give pgid to term
// Needs to be done in both parent and child to
// avoid race condition. Macro prevents code duplication and preserves
// assignment side effect without using pointers
#define Set_proc_group(JOB, PID, PGID)          \
    do {                                        \
        if (shell_is_interactive) {             \
            if (!PGID) PGID = PID;              \
            setpgid(PID, PGID);                 \
            if (!JOB->bkg)                      \
                tcsetpgrp(shell_term, PGID);    \
        }                                       \
    } while (0)

// Takes a job and returns the exit status of its last process
int launch_job(job *j)
{
    int io_fd[Arr_len(j->io)] = {0, 1, 2};
    // Open IO fds
    for (size_t i = 0; i < Arr_len(j->io); i++) {
        if (j->io[i].path) {
            io_fd[i] = open(j->io[i].path, j->io[i].oflag, DEF_MODE);
        }
        Stopif(io_fd[i] == -1, fd_cleanup(io_fd, i);
               return M_FAILED_IO, "%s", strerror(errno));
    }
    j->root->fds[0] = io_fd[0];
    for (cmd *crawler = j->root; crawler; crawler = crawler->next) {
        if (crawler->next) {
            int fd[2];
            pipe(fd);
            crawler->fds[1] = fd[1];
            crawler->next->fds[0] = fd[0];
        } else {
            // Alterations to stdout and stderr apply to last process
            crawler->fds[1] = io_fd[1];
            crawler->fds[2] = io_fd[2];
        }

        char **argv = crawler->argv->data;
        cmd_func f = find_node(argv[0], t);

        if (f) { // Builtin found
            crawler->exit_code = f(crawler);
            crawler->completed = 1;
        } else {
            pid_t p = fork();
            Stopif(p < 0, return M_FAILED_EXEC, "Could not fork process: %s",
                   strerror(errno));
            if (p == 0) { // Child
                p = getpid();
                Set_proc_group(j, p, j->pgid);
                reset_ignored_signals();
                exec_cmd(crawler);
            } else { // Parent
                Set_proc_group(j, p, j->pgid);
                crawler->pid = p;
            }
        }

        fd_cleanup(crawler->fds, Arr_len(io_fd));
    }

    if (!shell_is_interactive) {
        wait_for_job(j);
    } else if (j->bkg) {
        put_job_in_background(j, 0);
    } else {
        put_job_in_foreground(j, 0);
    }

    return 0;
}

static void exec_cmd(cmd const *c)
{
    char **argv = c->argv->data;
    char **env  = c->env->data;

    for (size_t i = 0; i < c->env->num; i++) {
        Stopif(putenv(env[i]) == -1, /* No action */,
               "Could not set the following variable/value pair: %s", env[i]);
    }

    for (size_t i = 0;  i < Arr_len(c->fds); i++) {
        dup2(c->fds[i], i);
    }

    Stopif(execvp(*argv, argv) == -1, exit(M_FAILED_EXEC),"%s: %s",
           strerror(errno), *argv);
}

// Wrapper function arround setpgid to reduce code duplication in parent and
// child processes;
static inline void set_proc_group(pid_t pid, pid_t *pgid)
{
    if (*pgid == 0) {
        *pgid = pid;
    }
    setpgid(pid, *pgid);
}

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
