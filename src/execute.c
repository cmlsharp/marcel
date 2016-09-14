#define _GNU_SOURCE // dprintf
#include <errno.h> // errno
#include <stdio.h> // close
#include <stdlib.h> // calloc, exit, putenv
#include <string.h> // strerror

#include <fcntl.h> // open, close
#include <sys/types.h> // pid_t
#include <unistd.h> // close, dup, getpid, setpgid, tcsetpgrp

#include "ds/proc.h" // proc, job
#include "ds/hash_table.h" // hash_table, add_node, find_node, free_table
#include "execute.h" // proc_func
#include "jobs.h" // interactive, shell_term, wait_for_job, put_job_in_*...
#include "macros.h" // Stopif, Free, Arr_len
#include "signals.h" // reset_signals

static void cleanup_builtins(void);
static void exec_proc(proc const *p);
static int m_cd(proc const *p);
static int m_exit(proc const *p);
static int m_help(proc const *p);

// Names of shell builtins
static char const *builtin_names[] = {
    "cd",
    "exit",
    "help",
};

// Functions associated with shell builtins
static proc_func const builtin_funcs[] = {
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
        if (interactive) {             \
            if (!PGID) PGID = PID;              \
            setpgid(PID, PGID);                 \
            if (!JOB->bkg)                      \
                tcsetpgrp(SHELL_TERM, PGID);    \
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
    for (proc *crawler = j->root; crawler; crawler = crawler->next) {
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
        proc_func f = find_node(argv[0], t);

        if (f) { // Builtin found
            crawler->exit_code = f(crawler);
            crawler->completed = 1;
        } else {
            pid_t p = fork();
            Stopif(p < 0, return M_FAILED_EXEC, "Could not fork process: %s",
                   strerror(errno));
            if (p == 0) { // Child
                Set_proc_group(j, p, j->pgid);
                reset_ignored_signals();
                exec_proc(crawler);
            } else { // Parent
                Set_proc_group(j, p, j->pgid);
                crawler->pid = p;
            }
        }

        fd_cleanup(crawler->fds, Arr_len(io_fd));
    }

    if (!interactive) {
        wait_for_job(j);
    } else if (j->bkg) {
        put_job_in_background(j, 0);
        format_job_info(j, "launched");
    } else {
        put_job_in_foreground(j, 0);
    }

    return 0;
}

static void exec_proc(proc const *p)
{
    char **argv = p->argv->data;
    char **env  = p->env->data;

    for (size_t i = 0; i < p->env->num; i++) {
        Stopif(putenv(env[i]) == -1, /* No action */,
               "Could not set the following variable/value pair: %s", env[i]);
    }

    for (size_t i = 0;  i < Arr_len(p->fds); i++) {
        dup2(p->fds[i], i);
    }

    // _Exit is used because cleanup_jobs is executed when `exit` is run and we
    // don't want to kill our other processes
    Stopif(execvp(*argv, argv) == -1, _Exit(M_FAILED_EXEC),"%s: %s",
           strerror(errno), *argv);
}

static int m_cd(proc const *p)
{
    // Avoid casting from void* at every use
    char **argv = p->argv->data;
    // cd to homedir if no directory specified
    char *dir = (argv[1]) ? argv[1] : getenv("HOME");
    Stopif(chdir(dir) == -1, return 1, "%s", strerror(errno));
    return 0;
}

static int m_exit(proc const *p)
{
    // Silence warnings about not using c
    (void) p;
    exit(exit_code);
}

static int m_help(proc const *p)
{
    char help_msg[] = "Marcel the Shell (with shoes on) v. " VERSION "\n"
                     "Written by Chad Sharp\n"
                     "\n"
                     "This shell only fights when provoked.\n";
    write(p->fds[1], help_msg, sizeof help_msg / sizeof (char));
    return 0;
}
