#ifndef MARCEL_H
#define MARCEL_H

#define NAME "marcel"
#define VERSION "1.0"
#define DEF_MODE 0666

// Most recent exit code
extern int exit_code;

// Dynamically allocated array that keeps track of its size

enum {
    M_SUCCESS = 0,
    M_SIGINT = -1,
    M_FAILED_INIT = -2,
    M_FAILED_EXEC = -3,
    M_FAILED_ALLOC = -4,
    M_FAILED_IO = -5,
    M_FAILED_PGID = -6,
};


#endif
