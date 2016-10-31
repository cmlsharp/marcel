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
#include <stdint.h> // uintptr_t
#include <string.h> // memset
#include "vec.h" // vec
#include "../macros.h" // Assert_alloc

typedef struct vec_meta {
    size_t cap;  // Allocated size in bytes
    size_t len;  // Length of vector
} vec_meta;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"

// Allocate zero-initialized vector (a dynamically allocated array with prefixed metadata) of `size` bytes
__attribute__((malloc))
vec vec_alloc(size_t size)
{
    vec_meta data = { .cap = size, .len = 0 };
    vec ret = malloc(sizeof data + size);
    Assert_alloc(ret);
    memcpy(ret, &data, sizeof data);
    ret = ((uintptr_t) ret) + sizeof data;
    return memset(ret, 0, size);
}

// NOTE: All the below functions REQUIRE that they be passed a vector. Their
// behavior is undefined otherwise

// Free vector
void vec_free(vec v)
{
    free((uintptr_t) v - sizeof (vec_meta));
}

// Return the allocated size of the vector in bytes
size_t vec_capacity(vec v)
{
    return ((vec_meta *)((uintptr_t) v - sizeof (vec_meta)))->cap;
}

// Returns the length (the number of occupied elements of the vector)
size_t vec_len(vec v)
{
    return ((vec_meta *)((uintptr_t) v - sizeof (vec_meta)))->len;
}

// Allows the client to manually change the length (POTENTIALLY UNSAFE)
void vec_setlen(size_t val, vec v)
{
    ((vec_meta *)((uintptr_t) v - sizeof (vec_meta)))->len = val;
}

// Add element to the end of a vector, growing it if necessary
int vec_append(void *elem, size_t elem_size, vec *v)
{
    vec_meta *data = (uintptr_t) *v - sizeof *data;
    if ((data->len + 1) * elem_size >= data->cap) {
        vec_grow(v);
        // Reinitialize data in case realloc changed the memory location
        data = (uintptr_t) *v - sizeof *data;
    }
    memcpy((uintptr_t) *v + data->len * elem_size, elem, elem_size);
    data->len++;
    return 0;
}

// Returns -1 if passed bad parameters, 1 if allocation failed or array
// capacity is already SIZE_MAX bytes. 0 on success
// 0 initializes newly allocated space
int vec_grow(vec *v)
{
    if (!v || !*v) {
        return -1;
    }

    vec ret = (uintptr_t) *v - sizeof (vec_meta);
    size_t bytes = ((vec_meta*) ret)->cap;
    if (bytes < SIZE_MAX / 2) {
        bytes *= 2;
    } else if (bytes < SIZE_MAX) {
        bytes = SIZE_MAX;
    } else {
        return 1;
    }
    ret = realloc(ret, sizeof (vec_meta) + bytes);
    Assert_alloc(ret);

    size_t *cap = &((vec_meta*) ret)->cap;
    memset(sizeof (vec_meta) + (uintptr_t) ret + *cap , 0, bytes - *cap);
    *cap = bytes;
    *v = (uintptr_t) ret + sizeof (vec_meta);
    return 0;
}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
