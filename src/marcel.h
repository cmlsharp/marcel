#ifndef MSH_H
#define MSH_H

#define NAME "marcel"
#define VERSION "1.0"
#define MAX_ARGS 256

#include <sys/types.h>

// This variable is modified only in the main function and the signal handling 
// function for SIGINT
extern int volatile exit_code;

// Struct to model a single command
typedef struct cmd {
    char *argv[MAX_ARGS]; // Command to execute and its arguments
    int in; // File descriptor of input
    int out; // File descriptor of output
    _Bool wait; // Wait for child process to finish
    struct cmd *next; // Pointer to next piped cmd
} cmd;

cmd *def_cmd(void);

#endif
