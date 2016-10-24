/*
 * Marcel the Shell -- a shell written in C
 * Copyright (C) 2016 Chad Sharp
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

// Default mode with which to create files
#define FILE_MASK 0666

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
hash_table lookup_table;

// Create hashtable of shell builtins
// Returns true on success, false on failure
_Bool initialize_builtins(void)
{
    lookup_table = new_table(TABLE_INIT_SIZE);
    // NOTE: We are mixing data pointers and function pointers here. ISO C
    // forbids this but it's fine in POSIX
    for (size_t i = 0; i < Arr_len(builtin_names); i++) {
        builtin *b = malloc(sizeof *b);
        Assert_alloc(b);
        b->type=CMD;
        b->cmd = builtin_funcs[i];
        if (add_node(builtin_names[i], b, lookup_table) != 0) {
            return 0;
        }
    }

    if (atexit(cleanup_builtins) != 0) {
        return 0;
    }
    return 1;
}

static inline void builtin_destructor(node *n) {
    free(n->value);
}

// Wrapper around free_table so it can be passed to atexit
static void cleanup_builtins(void)
{
    free_table(lookup_table, builtin_destructor);
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
        if (interactive) {                      \
            if (!PGID) PGID = PID;              \
            setpgid(PID, PGID);                 \
            if (!JOB->bkg)                      \
                tcsetpgrp(SHELL_TERM, PGID);    \
        }                                       \
    } while (0)

static inline _Bool filter_command(void *val) {
    builtin *b = val;
    return b->type == CMD;
}

// Takes a job and returns the exit status of its last process
int launch_job(job *j)
{
    int io_fd[] = {0, 1, 2};
    // Open IO fds
    for (size_t i = 0; i < Arr_len(j->io); i++) {
        if (j->io[i].path) {
            io_fd[i] = open(j->io[i].path, j->io[i].oflag, FILE_MASK);
        }
        Stopif(io_fd[i] == -1, fd_cleanup(io_fd, i);
               return M_FAILED_IO, "%s", strerror(errno));
    }
    size_t proc_len = vlen(j->procs);

    // Set input fd in first process
    j->procs[0]->fds[0] = io_fd[0];
    // Set output/error fds in last process
    j->procs[proc_len-1]->fds[1] = io_fd[1];
    j->procs[proc_len-1]->fds[2] = io_fd[2];

    for (proc **p_p = j->procs; p_p < j->procs + proc_len; p_p++) {
        proc *p = *p_p;
        // Do not create pipe for last process
        if (p_p != j->procs + proc_len - 1) {
            int fd[2];
            pipe(fd);
            p->fds[1] = fd[1];
            (p+1)->fds[0] = fd[0];
        }

        builtin *b = find_node(p->argv[0],filter_command, lookup_table);
        proc_func f = b ? b->cmd : NULL;

        if (f) { // Builtin found
            p->exit_code = f(p);
            p->completed = 1;
        } else {
            pid_t pid = fork();
            Stopif(pid < 0, return M_FAILED_EXEC, "Could not fork process: %s",
                   strerror(errno));
            if (pid == 0) { // Child
                Set_proc_group(j, pid, j->pgid);
                reset_ignored_signals();
                exec_proc(p);
            } else { // Parent
                Set_proc_group(j, pid, j->pgid);
                p->pid = pid;
            }
        }

        fd_cleanup(p->fds, Arr_len(io_fd));
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
    size_t env_len  = vlen(p->env);
    for (char **e_p = p->env; e_p < p->env + env_len; e_p++) {
        char *e = *e_p;
        char *value = e + strlen(e) + 1;
        unsetenv(e);
        Stopif(setenv(e, value, 1) == -1, /* No action */,
               "Could not set the following variable %s to %s", e, value);
    }

    for (size_t i = 0;  i < Arr_len(p->fds); i++) {
        dup2(p->fds[i], i);
    }

    // _Exit is used because cleanup_jobs is executed when `exit` is run and we
    // don't want to kill our other processes
    Stopif(execvp(*p->argv, p->argv) == -1, _Exit(M_FAILED_EXEC),"%s: %s",
           strerror(errno), *p->argv);
}

static int m_cd(proc const *p)
{
    // cd to homedir if no directory specified
    char *dir = (p->argv[1]) ? p->argv[1] : getenv("HOME");
    Stopif(chdir(dir) == -1, return 1, "%s", strerror(errno));
    return 0;
}

static int m_exit(proc const *p)
{
    // Silence warnings about not using p
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
