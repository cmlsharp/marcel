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
#include "vec.h" // vec
#include "../macros.h" // Assert_alloc

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

// Allocate array. Takes number of desired array members and the size of their
// type
vec new_vec(size_t nmemb, size_t size)
{
    vec ret = {.cap = nmemb, .num = 0, .size = size};

    ret.data = calloc(nmemb, size);
    Assert_alloc(ret.data);

    return ret;
}

void free_vec(vec *v)
{
    v->cap = v->num = 0;
    Free(v->data);
}

// Returns -1 if passed bad parameters, 1 if allocation failed or array
// capacity is already SIZE_MAX bytes. 0 on success
int grow_vec(vec *v)
{
    if (!v->data || !v->cap) {
        return -1;
    }

    size_t bytes = v->cap * v->size;
    if (bytes < SIZE_MAX / 2) {
        bytes *= 2;
    } else if (bytes < (SIZE_MAX - (SIZE_MAX % v->size))) {
        bytes  = SIZE_MAX - (SIZE_MAX % v->size);
    } else {
        return 1;
    }
    void *new_data = realloc(v->data, bytes);
    Assert_alloc(new_data);
    // initialize with zeros
    size_t new_cap = bytes/v->size;
    memset((char *)new_data + (v->size * v->cap), 0, (new_cap - v->cap) * v->size);
    v->cap = new_cap;
    v->data = new_data;
    return 0;
}
