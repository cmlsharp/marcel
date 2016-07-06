#include <stdlib.h>
#include "dyn_array.h"
#include "../macros.h"

// Allocate array. Takes number of desired array members and the size of their
// type
dyn_array *new_dyn_array(size_t nmemb, size_t size)
{
    dyn_array *ret = malloc(sizeof *ret);
    Assert_alloc(ret);

    ret->data = calloc(nmemb, size);
    Assert_alloc(ret->data);

    ret->cap = nmemb;
    ret->num = 0;
    return ret;
}

void free_dyn_array(dyn_array *d) {
    Free(d->data);
    Free(d);
}
