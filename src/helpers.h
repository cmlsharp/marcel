#ifndef MARCELL_HELPER_H
#define MARCELL_HELPER_H
// Misc helper functions

#include <stddef.h> // size_t

// Doubles array until SIZE_MAX and zero initializes newly allocated memory
void *grow_array(void *arr, size_t *cap);
#endif
