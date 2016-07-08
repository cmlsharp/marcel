#include <stdlib.h> // calloc, realloc
#include <string.h> // memset
#include "dyn_array.h" // dyn_array
#include "../macros.h" // Assert_alloc, Cast

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

// Allocate array. Takes number of desired array members and the size of their
// type
dyn_array *new_dyn_array(size_t nmemb, size_t size)
{
    dyn_array *ret = Cast(dyn_array*) malloc(sizeof *ret);
    Assert_alloc(ret);

    ret->data = calloc(nmemb, size);
    Assert_alloc(ret->data);

    ret->cap = nmemb;
    ret->num = 0;
    ret->size = size;
    return ret;
}

void free_dyn_array(dyn_array *d) {
    Free(d->data);
    Free(d);
}

// Returns -1 if passed bad parameters, 1 if allocation failed or array
// capacity is already SIZE_MAX bytes. 0 on success
int grow_dyn_array(dyn_array *d)
{
    if (!d->data || !d->cap) {
        return -1;
    }

    size_t bytes = d->cap * d->size;
    if (bytes < SIZE_MAX / 2) {
        bytes *= 2;
    } else if (bytes < (SIZE_MAX - (SIZE_MAX % d->size))) {
        bytes  = SIZE_MAX - (SIZE_MAX % d->size);
    } else {
        return 1;
    }
    void *new_data;
    if (!(new_data = realloc(d->data, bytes))) {
        return 1;
    }
    // initialize with zeros
    size_t new_cap = bytes/d->size;
    memset((char *)new_data + (d->size * d->cap), 0, (new_cap - d->cap) * d->size);
    d->cap = new_cap;
    d->data = new_data;
    return 0;
    
}
