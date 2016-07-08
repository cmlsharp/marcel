#ifndef MARCELL_MACROS_H
#define MARCELL_MACROS_H

#include "marcel.h"
#include <stdio.h> // fprintf
#include <errno.h> // errno
#include <string.h> // strerror

// Length of array. ARR cannot be a pointer
#define Arr_len(ARR) (sizeof (ARR) / sizeof *(ARR))

// Standard way to print error messages across program
#define Err_msg(...)                                                    \
    do {                                                                \
        fprintf(stderr, "%s: Provocation detected: ", NAME);            \
        fprintf(stderr, __VA_ARGS__);                                   \
        fprintf(stderr, "\n");                                          \
    } while (0)
        

// Make error handling easier
#define Stopif(COND, ACTION, ...)                                           \
    do {                                                                    \
        if (COND) {                                                         \
            Err_msg(__VA_ARGS__);                                           \
            ACTION;                                                         \
        }                                                                   \
    } while (0)


#define Assert_alloc(PTR)                                                   \
    Stopif(!(PTR),                                                          \
           _Exit(M_FAILED_ALLOC),                                           \
           "Fatal error encountered. Quitting. System reports %s",          \
           strerror(errno))


// More general version of Free. Allows for custom destructor
// NOTE: _F(NULL) must be defined behavior for this macro to serve its purpose
#define Cleanup(PTR, F)                                                     \
    do {                                                                    \
        F(PTR);                                                             \
        PTR = NULL;                                                         \
    } while (0)

// Stop double frees/use after frees
#define Free(PTR) Cleanup (PTR, free)

// Explicitly cast pointers if compiling as C++
#ifdef __cplusplus
#define Cast(TYPE) (TYPE)
#else
#define Cast(TYPE)
#endif

// GCC pops twice for every push to reset to commandline specified option
#if defined (__GNUC__) && ((__GNUC__ > 3) || (defined(__GNUC_MINOR__) && (__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
// GCC
#define GCC_POP _Pragma("GCC diagnostic pop"); _Pragma("GCC diagnostic pop")
#else
#define GCC_POP _Pragma("GCC diagnostic pop");
#endif

// Symmetry with GCC_POP
#define GCC_PUSH _Pragma("GCC diagnostic push")

#endif
