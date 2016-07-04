#include <stdlib.h> // realloc
#include <string.h> // memset

#include "helpers.h" // size_t

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

void *grow_array(void *arr, size_t *cap)
{
    if (!arr || !cap || !*cap) {
        return NULL;
    }
    size_t new_cap = *cap;
    if (new_cap < SIZE_MAX / 2) {
        new_cap *= 2;
    } else if (new_cap < SIZE_MAX) {
        new_cap = SIZE_MAX;
    } else {
        return NULL;
    }
    void *ret;
    if ((ret = realloc(arr, new_cap))) {
        // initialize with zeros
        memset(ret + *cap, 0, new_cap - *cap);
        *cap = new_cap;
    }
    return ret;
}
