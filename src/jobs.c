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

#include <stdlib.h> // atexit
#include <string.h> // strerror
#include <errno.h> // errno

#include <signal.h> // kill
#include <sys/types.h> // pid_t
#include <sys/wait.h> // waitpid
#include <termios.h> // termios, TCSADRAIN
#include <unistd.h> // getpgid, tcgetpgrp, tcsetpgrp, getpgrp...

#include "ds/proc.h" // job, free_single_job, proc
#include "ds/vec.h" // dyn_arrray, valloc
#include "jobs.h" // function prototypes
#include "signals.h" // sig_flags, WAITING_FOR_INPUT
#include "macros.h" // Cleanup, Stopif, Err_msg

#ifndef WAIT_ANY
#define WAIT_ANY -1
#endif

#define JOB_TABLE_INIT_SIZE 256

_Bool interactive;
static job **job_table;
static pid_t shell_pgid;
static struct termios shell_tmodes;

static void cleanup_jobs(void);

// Put shell in forground if interactive
// Returns true on success, false on failure
_Bool initialize_job_control(void)
{
    job_table = valloc(JOB_TABLE_INIT_SIZE * sizeof *job_table);
    interactive = isatty(SHELL_TERM);
    if (interactive) {
        // Loop until in foreground
        while ((shell_pgid = getpgrp()) != tcgetpgrp(SHELL_TERM)) {
            kill(-shell_pgid, SIGTTIN);
        }
        // Put in own process group
        shell_pgid = getpid();
        Stopif(setpgid(shell_pgid, shell_pgid) < 0, return 0,
               "Couldn't put shell in its own process group");

        // Get control of terminal
        tcsetpgrp(SHELL_TERM, shell_pgid);
        // Save default term attributes
        tcgetattr(SHELL_TERM, &shell_tmodes);
    }
    atexit(cleanup_jobs);
    return 1;
}

// Free job table and kill all background jobs
static void cleanup_jobs(void)
{
    size_t len = vlen(job_table);
    for (job **j_p = job_table; j_p < job_table + len; j_p++) {
        job *j = *j_p;
        if (!j) {
            continue;
        }

        if (j->bkg) {
            kill(j->pgid, SIGHUP);
        } else {
            wait_for_job(j);
        }

        free_single_job(j);
    }
    vfree(job_table);
}

// Put job in foreground, continuing if cont is true
void put_job_in_foreground(job *j, _Bool cont)
{
    // Put job in foreground
    tcsetpgrp(SHELL_TERM, j->pgid);
    // Send SIGCONT if necessary
    if (cont) {
        tcsetattr(SHELL_TERM, TCSADRAIN, &j->tmodes);
        Stopif(kill(-j->pgid, SIGCONT) < 0, /* No action */,
               "Error continuing process: %s", strerror(errno));
    }
    wait_for_job(j);
    // Put shell in foreground
    tcsetpgrp(SHELL_TERM, shell_pgid);

    // Restore terminal modes
    tcgetattr(SHELL_TERM, &j->tmodes);
    tcsetattr(SHELL_TERM, TCSADRAIN, &shell_tmodes);

}

// Put job in background, send SIGCONT if cont is true
void put_job_in_background(job *j, _Bool cont)
{
    // Send SIGCONT if necessary
    if (cont) {
        Stopif(kill(-j->pgid, SIGCONT) < 0, /* No action */, "%s", strerror(errno));
    }
}

// Find proc that corresponds with pid and mark it as stopped or completed as
// apropriate.  Return true on success, false on failure
_Bool mark_proc_status(pid_t pid, int status)
{
    if (pid > 0) {
        size_t job_len = vlen(job_table);
        for (job **j_p = job_table; j_p < job_table + job_len; j_p++) {
            job *j = *j_p;
            if (!j) {
                continue;
            }
            size_t proc_len = vlen(j->procs);
            for (proc **p_p = j->procs; p_p < j->procs + proc_len; p_p++) {
                proc *p = *p_p;
                if (p->pid == pid) {
                    if (WIFSTOPPED(status) || WIFCONTINUED(status)) {
                        p->stopped = !p->stopped;
                        j->notified = 0;
                    } else {
                        p->exit_code = (WIFSIGNALED(status)) ? M_SIGINT : WEXITSTATUS(status);
                        p->completed = 1;
                    }
                return 1;
                }
            }
        }
        Err_msg("No child process %d", pid);
        return 0;
    } else if (pid == 0 || errno == ECHILD) {
        // No processes available to report
        return 0;
    } else {
        Err_msg("Error waiting on child process");
        return 0;
    }
}

// Check for processes with statuses to report (without blocking)
void update_status(void)
{
    int status = 0;
    pid_t pid = 0;
    do {
        pid = waitpid(WAIT_ANY, &status, WUNTRACED | WNOHANG | WCONTINUED);
    } while (mark_proc_status(pid, status));
}

// Check for processes with statuses to report (blocking)
void wait_for_job(job *j)
{
    int status;
    pid_t pid;
    do {
        pid = waitpid(-j->pgid, &status, WUNTRACED | WCONTINUED);
    } while (mark_proc_status(pid, status)
             && !job_is_stopped(j)
             && !job_is_completed(j));

}

void format_job_info(job *j, char const *msg)
{
    fprintf(stderr, "[%zu] %d (%s): %s\n", j->index+1, j->pgid,  msg, j->name);
}

// Notify user of changes in job status, free job if completed
// Return exit code of the completed job that was launched most recently
int do_job_notification(void)
{
    update_status();
    int ret = 0;
    size_t job_len = vlen(job_table);
    for (job **j_p = job_table; j_p < job_table + job_len; j_p++) {
        job *j = *j_p;
        if (!j) {
            continue;
        }
        // If all procs have completed, job is completed
        if (job_is_completed(j)) {
            // Only notify about background jobs
            if (j->bkg) {
                format_job_info(j, "completed");
            }
            // Get exit code from last process in
            ret = j->procs[vlen(j->procs) - 1]->exit_code;

            free_single_job(j);
            *j_p = NULL;
        } else if (job_is_stopped(j) && !j->notified) {
            format_job_info(j, "stopped");
            j->notified = 1;
        } else {
        }
    }
    return ret;

}

// Mark stopped job as running
static void mark_job_as_running(job *j)
{
    size_t proc_len = vlen(j->procs);
    for (proc **p_p = j->procs; p_p < proc_len + j->procs; p_p++) {
        (*p_p)->stopped = 0;
    }

    j->notified = 0;
}

void continue_job(job *j)
{
    mark_job_as_running(j);
    if (j->bkg) {
        put_job_in_background(j, 1);
    } else {
        put_job_in_foreground(j, 1);
    }
}

// Return true if all processes in job have stopped or completed
_Bool job_is_stopped(job *j)
{
    size_t proc_len = vlen(j->procs);
    for (proc **p_p = j->procs; p_p < j->procs + proc_len; p_p++) {
        proc *p = *p_p;
        if (!p->completed && !p->stopped) {
            return 0;
        }
    }
    return 1;
}

// Check if all processes in job are completed
_Bool job_is_completed(job *j)
{
    size_t proc_len = vlen(j->procs);
    for (proc **p_p = j->procs; p_p < proc_len + j->procs; p_p++) {
        if (!(*p_p)->completed) {
            return 0;
        }
    }
    return 1;
}

// Add job to global job list, return false if array needs to be grown, but
// could not be or if job table has not been initialized
_Bool register_job(job *j)
{
    if (!job_table) {
        return 0;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    size_t job_cap = vcapacity(job_table) / sizeof *job_table;
    if ((vlen(job_table) + 1 >= job_cap)
            && vgrow(&job_table) != 0) {
        return 0;
    }
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

    for (job **j_p = job_table; j_p < job_table + job_cap; j_p++) { 
        if (!*j_p) {
            *j_p = j;
            size_t i = j_p - job_table;
            j->index = i;
            if (i >= vlen(job_table)) {
                vsetlen(vlen(job_table) + 1, job_table);
            }
            return 1;
        }

    }

    return 0;

}
