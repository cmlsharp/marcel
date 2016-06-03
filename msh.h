#ifndef MSH_H
#define MSH_H

#define NAME "msh"
#define MAX_ARGS 256

// Unfortunately this is global for simplicity's sake
extern int exit_code;

typedef struct cmd {
    char *argv[MAX_ARGS];
    int in;
    int out;
} cmd;

// Command with empty buffer, input from stdin, output to stdout
#define CMD_DEF {.argv = {0}, .in = 0, .out = 1}

#endif
