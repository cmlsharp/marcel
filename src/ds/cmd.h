#ifndef M_CMD_H
#define M_CMD_H

#define ARGV_INIT_SIZE 1024

#include <sys/types.h>
#include <termios.h>
#include "dyn_array.h"

// Struct to model a single command (process)
typedef struct cmd {
    dyn_array *argv; // Arguments to be passed to execvp
    dyn_array *env; // Environment variables in the form "VAR=VALUE"
    pid_t pid; // Pid of command
    int fds[3]; // File descriptors for input, output, error
    _Bool completed; // Command has finished executing
    _Bool stopped; // Command has been stopped
    int exit_code; // Status code cmd exited with
    struct cmd *next; // Pointer to next piped cmd
} cmd;

cmd *new_cmd(void);
void free_cmd(cmd *c);


typedef struct cmd_io {
    char *path;
    int oflag;
} cmd_io;

typedef struct job {
    struct job *next;
    char *name; // Name of command
    size_t index; // Index in job table
    cmd *root; // First command
    cmd_io io[3]; // stdin, stdout and stderr
    pid_t pgid; // Proc group ID for job
    _Bool notified; // User has been notified of state change
    _Bool bkg; // Job should execute in background
    struct termios tmodes; // Terminal modes for job
} job;

job *new_job(void);
void free_single_job(job *j);

#endif
