#ifndef MARCEL_H
#define MARCEL_H

#define NAME "marcel"
#define VERSION "1.0"
#define ARGV_INIT_SIZE 1024
#define DEF_MODE 0666

// This variable is modified only in the main function and the signal handling 
// function for SIGINT
extern int volatile exit_code;

// Dynamically allocated array of strings
typedef struct str_array {
    char **strs;
    size_t cap; // Size of dynamically allocated array
    size_t num; // Number of ocupied slots
} str_array;

// Struct to model a single command
typedef struct cmd {
    str_array argv; // Arguments to be passed to execvp
    int fds[3]; // File descriptors for input, output, error
    _Bool wait; // Wait for child process to finish
    str_array env; // Environment variables in the form "VAR=VALUE"
    struct cmd *next; // Pointer to next piped cmd
} cmd;

typedef struct cmd_io {
    char *path;
    int oflag;
} cmd_io;

typedef struct cmd_wrapper {
    cmd_io io[3];
    cmd *root;
} cmd_wrapper;

enum {
    M_SUCCESS = 0,
    M_SIGINT = -1,
    M_FAILED_EXEC = -2,
    M_FAILED_ALLOC = -3,
    M_FAILED_IO = -4,
};

cmd *new_cmd(void);
void free_cmd(cmd *c);

#endif
