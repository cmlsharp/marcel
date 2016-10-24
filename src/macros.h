/*
 * Marcel the Shell -- a shell written in C
 * Copyright (C) 2016 Chad Sharp
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MARCELL_MACROS_H
#define MARCELL_MACROS_H

#include "marcel.h"
#include <stdio.h> // fprintf
#include <errno.h> // errno
#include <string.h> // strerror

// Use GCC specific attributes if they are available, ignore them if not
#ifndef __attribute__
#define __attribute__(X)
#endif

// Length of array. ARR cannot be a pointer
#define Arr_len(ARR) (sizeof (ARR) / sizeof *(ARR))

#ifndef NDEBUG
#define Error_prefix "%s (%s:%d): ", NAME, __FILE__, __LINE__
#else
#define Error_prefix "%s: ", NAME
#endif

// Standard way to print error messages across program
#define Err_msg(...)                                                        \
    do {                                                                    \
        fprintf(stderr, Error_prefix);                                      \
        fprintf(stderr, __VA_ARGS__);                                       \
        fprintf(stderr, "\n");                                              \
    } while (0)

// Make error handling easier
#define Stopif(COND, ACTION, ...)                                           \
    do {                                                                    \
        if (COND) {                                                         \
            Err_msg(__VA_ARGS__);                                           \
            ACTION;                                                         \
        }                                                                   \
    } while (0)


// Special case of Stopif for allocation errors
#ifndef NDEBUG
#define Assert_alloc(PTR)                                                   \
    Stopif(!(PTR),                                                          \
           exit(M_FAILED_ALLOC),                                            \
           "Fatal error encountered. Quitting. System reports %s",          \
           strerror(errno))
#else
#define Assert_alloc(PTR)
#endif

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
