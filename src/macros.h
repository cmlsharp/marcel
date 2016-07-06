#ifndef MARCELL_MACROS_H
#define MARCELL_MACROS_H

#include "marcel.h"
#include <stdio.h> // fprintf
#include <errno.h> // errno

// Length of array. ARR cannot be a pointer
#define Arr_len(ARR) (sizeof (ARR) / sizeof *(ARR))

// Iterate over an array. Modefied version of one found on stackoverflow
#define I_Foreach(ITEM, LIST)                                               \
    for(size_t KEEP = 1, COUNT = 0, SIZE = Arr_len((LIST));                 \
        KEEP && COUNT != SIZE;                                              \
        KEEP = !KEEP, COUNT++)                                              \
        for (ITEM = (LIST) + COUNT; KEEP; KEEP = !KEEP)                     \

// Syntatic sugar for Foreach.
// NAME is the name of the pointer that will point to each member of the list
// TYPE is the type of values you want to iterate over
// The rest of the values are items you want to include in the list.
// Example:
// Foreach(int, i_ptr, 1, 2, 3) {
//     printf("%d", *i_ptr); 
// }
// 
// Output:
// 123

#define Foreach(TYPE, NAME, ...) I_Foreach(TYPE *NAME, ((TYPE[]) {__VA_ARGS__})) 

// Standard way to print error messages across program
#define Err_msg(...)                                                    \
    do {                                                                \
        fprintf(stderr, "%s: ", NAME);                                  \
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

#endif
