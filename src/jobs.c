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
#include "ds/vec.h" // dyn_arrray, vec_alloc
#include "jobs.h" // function prototypes
#include "signals.h" // sig_flags, WAITING_FOR_INPUT
#include "macros.h" // Cleanup, Stopif, Err_msg

#ifndef WAIT_ANY
#define WAIT_ANY -1
#endif

#define JOB_TABLE_INIT_SIZE 256

bool interactive;
static job **job_table;
static pid_t shell_pgid;
static struct termios shell_tmodes;

static void cleanup_jobs(void);

// Put shell in forground if interactive
// Returns true on success, false on failure
bool initialize_job_control(void)
{
    job_table = vec_alloc(JOB_TABLE_INIT_SIZE * sizeof *job_table);
    interactive = isatty(SHELL_TERM);
    if (interactive) {
        // Loop until in foreground
        while ((shell_pgid = getpgrp()) != tcgetpgrp(SHELL_TERM)) {
            kill(-shell_pgid, SIGTTIN);
        }
        // Put in own process group
        shell_pgid = getpid();
        Stopif(setpgid(shell_pgid, shell_pgid) < 0, return false,
               "Couldn't put shell in its own process group");

        // Get control of terminal
        tcsetpgrp(SHELL_TERM, shell_pgid);
        // Save default term attributes
        tcgetattr(SHELL_TERM, &shell_tmodes);
    }
    atexit(cleanup_jobs);
    return true;
}

// Free job table and kill all background jobs
static void cleanup_jobs(void)
{
    job **end = job_table + vec_len(job_table);
    for (job **j_p = job_table; j_p != end; j_p++) {
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
    vec_free(job_table);
}

// Put job in foreground, continuing if cont is true
void send_to_foreground(job *j, bool cont)
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
void send_to_background(job *j, bool cont)
{
    // Send SIGCONT if necessary
    if (cont) {
        Stopif(kill(-j->pgid, SIGCONT) < 0, /* No action */, "%s", strerror(errno));
    }
}

// Find proc that corresponds with pid and mark it as stopped or completed as
// apropriate.  Return true on success, false on failure
// TODO: Extend to returning information about other kinds of signals
bool mark_proc_status(pid_t pid, int status)
{
    if (pid > 0) {
        job **job_end = job_table + vec_len(job_table);
        for (job **j_p = job_table; j_p != job_end; j_p++) {
            job *j = *j_p;
            if (!j) {
                continue;
            }
            proc **proc_end = vec_len(j->procs) + j->procs;
            for (proc **p_p = j->procs; p_p != proc_end; p_p++) {
                proc *p = *p_p;
                if (p->pid == pid) {
                    if (WIFSTOPPED(status) || WIFCONTINUED(status)) {
                        p->stopped = !p->stopped;
                        j->notified = false;
                    } else {
                        p->exit_code = (WIFSIGNALED(status)) ? M_SIGINT : WEXITSTATUS(status);
                        p->completed = true;
                    }
                    return true;
                }
            }
        }
        Err_msg("No child process %d", pid);
        return false;
    } else if (pid == 0 || errno == ECHILD) {
        // No processes available to report
        return false;
    } else {
        Err_msg("Error waiting on child process");
        return false;
    }
}

// Check for processes with statuses to report (without blocking)
void check_job_status(void)
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
             && !is_stopped(j)
             && !is_completed(j));

}

void format_job_info(job *j, char const *msg)
{
    fprintf(stderr, "[%zu] %d (%s): %s\n", j->index+1, j->pgid,  msg, j->name);
}

// Notify user of changes in job status, free job if completed
// Return exit code of the completed job that was launched most recently
int report_job_status(void)
{
    check_job_status();
    int ret = 0;
    job **job_end = job_table + vec_len(job_table);
    for (job **j_p = job_table; j_p != job_end; j_p++) {
        job *j = *j_p;
        if (!j) {
            continue;
        }
        // If all procs have completed, job is completed
        if (is_completed(j)) {
            // Only notify about background jobs
            if (j->bkg) {
                format_job_info(j, "completed");
            }
            // Get exit code from last process in
            ret = j->procs[vec_len(j->procs) - 1]->exit_code;

            // We are removing last job, calculate new last job index
            if (j_p + 1 >= job_end) {
                job **j_crawl = j_p - 1;
                for (; j_crawl >= job_table && !*j_crawl; j_crawl--);
                vec_setlen(j_crawl - job_table + 1, job_table);
            }
            free_single_job(j);
            *j_p = NULL;
        } else if (is_stopped(j) && !j->notified) {
            format_job_info(j, "stopped");
            j->notified = true;
        }
    }
    return ret;

}

// Mark stopped job as running
static void mark_running(job *j)
{
    proc **proc_end = j->procs + vec_len(j->procs);
    for (proc **p_p = j->procs; p_p != proc_end; p_p++) {
        (*p_p)->stopped = false;
    }

    j->notified = false;
}

void continue_job(job *j)
{
    mark_running(j);
    if (j->bkg) {
        send_to_background(j, true);
    } else {
        send_to_foreground(j, true);
    }
}

// Return true if all processes in job have stopped or completed
bool is_stopped(job *j)
{
    proc **proc_end = j->procs + vec_len(j->procs);
    for (proc **p_p = j->procs; p_p != proc_end; p_p++) {
        proc *p = *p_p;
        if (!p->completed && !p->stopped) {
            return false;
        }
    }
    return true;
}

// Check if all processes in job are completed
bool is_completed(job *j)
{
    proc **proc_end = j->procs + vec_len(j->procs);
    for (proc **p_p = j->procs; p_p != proc_end; p_p++) {
        if (!(*p_p)->completed) {
            return false;
        }
    }
    return true;
}

// Add job to global job list, return false if job table needs to expand, but
// expansion failed or if job table has not been initialized
bool register_job(job *j)
{
    if (!job_table) {
        return false;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    size_t job_cap = vec_capacity(job_table) / sizeof *job_table;
    // Grow job table if necessary
    if (vec_len(job_table) + 1 >= job_cap
            && vec_grow(&job_table) != 0) {
        return false;
    }
#pragma GCC diagnostic pop

    // Find empty slot in table to place job
    job **job_end = job_table + job_cap;
    for (job **j_p = job_table; j_p != job_end; j_p++) {
        if (!*j_p) {
            *j_p = j;
            size_t i = j_p - job_table;
            j->index = i;
            if (i >= vec_len(job_table)) {
                vec_setlen(vec_len(job_table) + 1, job_table);
            }
            return true;
        }

    }

    return false;

}
