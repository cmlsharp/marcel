#ifndef M_CMD_H
#define M_CMD_H

#define ARGV_INIT_SIZE 1024

#include "dyn_array.h"
// Struct to model a single command
typedef struct cmd {
    dyn_array *argv; // Arguments to be passed to execvp
    dyn_array *env; // Environment variables in the form "VAR=VALUE"
    int fds[3]; // File descriptors for input, output, error
    _Bool wait; // Wait for child process to finish
    struct cmd *next; // Pointer to next piped cmd
} cmd;

cmd *new_cmd(void);
void free_cmd(cmd *c);


typedef struct cmd_io {
    char *path;
    int oflag;
} cmd_io;

typedef struct cmd_wrapper {
    cmd_io io[3];
    cmd *root;
} cmd_wrapper;

cmd_wrapper *new_cmd_wrapper(void);
void free_cmd_wrapper(cmd_wrapper *w);
#endif
