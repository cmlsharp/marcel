#define _POSIX_SOURCE // kill
#include <stdlib.h> // atexit
#include <string.h> // strerror
#include <errno.h> // errno

#include <signal.h> // kill
#include <sys/types.h> // pid_t
#include <sys/wait.h> // waitpid
#include <termios.h> // termios, TCSADRAIN
#include <unistd.h> // getpgid, tcgetpgrp, tcsetpgrp, getpgrp...

#include "ds/cmd.h" // job, free_single_job, cmd
#include "ds/dyn_array.h" // dyn_arrray, new_dyn_array
#include "jobs.h" // function prototypes
#include "signals.h" // sig_flags, WAITING_FOR_INPUT
#include "macros.h" // Cleanup, Stopif, Err_msg

#ifndef WAIT_ANY
#define WAIT_ANY -1
#endif

#define JOB_TABLE_INIT_SIZE 256

_Bool interactive;
int shell_term;
static dyn_array *job_table;
static pid_t shell_pgid;
static struct termios shell_tmodes;

static void cleanup_job_control(void);

// Put shell in forground if interactive
// Returns true on success, false on failure
_Bool initialize_job_control(void)
{
    job_table = new_dyn_array(JOB_TABLE_INIT_SIZE, sizeof (job *));
    shell_term = STDIN_FILENO;
    interactive = isatty(shell_term);
    if (interactive) {
        // Loop until in foreground
        while (tcgetpgrp(shell_term) != (shell_pgid = getpgrp())) {
            kill(-shell_pgid, SIGTTIN);
        }
        // Put in own process group
        shell_pgid = getpid();
        Stopif(setpgid(shell_pgid, shell_pgid) < 0, return 0,
               "Couldn't put shell in its own process group");

        // Get control of terminal
        tcsetpgrp(shell_term, shell_pgid);
        // Save default term attributes
        tcgetattr(shell_term, &shell_tmodes);
    }
    atexit(cleanup_job_control);
    return 1;
}

static void cleanup_job_control(void)
{
    job **jobs = job_table->data;
    for (size_t i = 0; i < job_table->num; i++) {
        if (!jobs[i]) {
            continue;
        }
        kill(jobs[i]->pgid, SIGHUP);
        free_single_job(jobs[i]);
    }
    Cleanup(job_table, free_dyn_array);
}

// Put job in foreground, continuing if cont is true
void put_job_in_foreground(job *j, _Bool cont)
{
    // Put job in foreground
    tcsetpgrp(shell_term, j->pgid);
    // Send SIGCONT if necessary
    if (cont) {
        tcsetattr(shell_term, TCSADRAIN, &j->tmodes);
        Stopif(kill(-j->pgid, SIGCONT) < 0, /* No action */,
               "Error continuing process: %s", strerror(errno));
    }
    wait_for_job(j);
    // Put shell in foreground
    tcsetpgrp(shell_term, shell_pgid);

    // Restore terminal modes
    tcgetattr(shell_term, &j->tmodes);
    tcsetattr(shell_term, TCSADRAIN, &shell_tmodes);

}

// Put job in background, send SIGCONT if cont is true
void put_job_in_background(job *j, _Bool cont)
{
    // Send SIGCONT if necessary
    if (cont) {
        Stopif(kill(-j->pgid, SIGCONT) < 0, /* No action */, "%s", strerror(errno));
    }
}

// Find cmd that corresponds with pid and mark it as stopped or completed as
// apropriate.  Return true on success, false on failure
_Bool mark_cmd_status(pid_t pid, int status)
{
    if (pid > 0) {
        job **jobs = job_table->data;
        for (size_t i = 0; i < job_table->num; i++) {
            if (!jobs[i]) {
                continue;
            }
            for (cmd *c = jobs[i]->root; c; c = c->next) {
                if (c->pid == pid) {
                    if (WIFSTOPPED(status) || WIFCONTINUED(status)) {
                        c->stopped = !c->stopped;
                        jobs[i]->notified = 0;
                    } else {
                        c->exit_code = (WIFSIGNALED(status)) ? M_SIGINT : WEXITSTATUS(status);
                        c->completed = 1;
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

// Check for processes with statuses to report without blocking
void update_status(void)
{
    int status = 0;
    pid_t pid = 0;
    do {
        pid = waitpid(WAIT_ANY, &status, WUNTRACED | WNOHANG | WCONTINUED);
    } while (mark_cmd_status(pid, status));
}

void wait_for_job(job *j)
{
    int status;
    pid_t pid;
    do {
        pid = waitpid(-j->pgid, &status, WUNTRACED | WCONTINUED);
    } while (mark_cmd_status(pid, status)
             && !job_is_stopped(j)
             && !job_is_completed(j));

}

void format_job_info(job *j, char const *msg)
{
    fprintf(stderr, "[%zu] (%s): %s\n", j->index+1, msg, j->name);
}

// Notify user of changes in job status, free job if completed
// Return exit code of the completed job that was launched most recently
int do_job_notification(void)
{
    update_status();
    int ret = 0;
    job **jobs = job_table->data;
    for (size_t i = 0; i < job_table->num; i++) {
        if (!jobs[i]) {
            continue;
        }
        // If all cmds have completed, job is completed
        if (job_is_completed(jobs[i])) {
            // Only notify about background jobs
            if (jobs[i]->bkg) {
                format_job_info(jobs[i], "completed");
            }
            // Get exit code from last process in
            cmd *c;
            for (c = jobs[i]->root; c && c->next; c = c->next);
            ret = c->exit_code;

            Cleanup(jobs[i], free_single_job);
        } else if (job_is_stopped(jobs[i]) && !jobs[i]->notified) {
            format_job_info(jobs[i], "stopped");
            jobs[i]->notified = 1;
        } else {
        }
    }
    return ret;

}

// Mark stopped job as running
static void mark_job_as_running(job *j)
{
    for (cmd *c = j->root; c; c = c->next) {
        c->stopped = 0;
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
    cmd *c;
    for (c = j->root; c; c = c->next) {
        if (!c->completed && !c->stopped) {
            return 0;
        }
    }
    return 1;
}

// Check if all processes in job are completed
_Bool job_is_completed(job *j)
{
    cmd *c;
    for (c = j->root; c; c = c->next) {
        if (!c->completed) {
            return 0;
        }
    }
    return 1;
}

// Add job to global job list, return false if array needs to be grown, but
// could not be or if job table has not been initialized
_Bool register_job(job *j)
{
    if (!job_table || !job_table->data) {
        goto fail;
    }

    if (job_table->num + 1 >= job_table->cap && grow_dyn_array(job_table) != 0) {
        goto fail;
    }

    job **jobs = job_table->data;
    for (size_t i = 0; i < job_table->cap; i++) { 
        if (!jobs[i]) {
            jobs[i] = j;
            j->index = i;
            job_table->num++;
            return 1;
        }

    }

fail:
    return 0;
}
