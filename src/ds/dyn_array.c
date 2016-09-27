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

#include <stdlib.h> // calloc, realloc
#include <string.h> // memset
#include "dyn_array.h" // dyn_array
#include "../macros.h" // Assert_alloc

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

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
    ret->size = size;
    return ret;
}

void free_dyn_array(dyn_array *d)
{
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
    void *new_data = realloc(d->data, bytes);
    Assert_alloc(new_data);
    // initialize with zeros
    size_t new_cap = bytes/d->size;
    memset((char *)new_data + (d->size * d->cap), 0, (new_cap - d->cap) * d->size);
    d->cap = new_cap;
    d->data = new_data;
    return 0;
}
