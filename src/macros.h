#ifndef STOPIF_H
#define STOPIF_H

#include <stdio.h> // fprintf

// Length of array. _ARR cannot be a pointer
#define Arr_len(_ARR) (sizeof _ARR / sizeof *_ARR)

// Make error handling easier
#define Stopif(_COND, _ACTION, ...)                                         \
    do {                                                                    \
        if (_COND) {                                                        \
            fprintf(stderr, "%s: ", NAME);                                  \
            fprintf(stderr, __VA_ARGS__);                                   \
            fprintf(stderr, "\n");                                          \
            _ACTION;                                                        \
        }                                                                   \
    } while (0)


// More general version of Free. Allows for custom destructor
// NOTE: _F(NULL) must be defined behavior for this macro to serve its purpose
#define Cleanup(_PTR, _F)                                                   \
    do {                                                                    \
        _F(_PTR);                                                           \
        _PTR = NULL;                                                        \
    } while (0)

// Stop double frees/use after frees
#define Free(_PTR) Cleanup (_PTR, free)

// Free multiple pointers at once
#define Free_all(...)                                                       \
    do {                                                                    \
        void *_PTRS[] = {__VA_ARGS__};                                      \
        for (size_t _I = 0; _I < (Arr_len(_PTRS)); _I++) {  \
            Free(_PTRS[_I]);                                                \
        }                                                                   \
    } while (0)

#endif
