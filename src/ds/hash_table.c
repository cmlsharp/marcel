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

#include <stdlib.h> // malloc
#include <string.h> // strcmp

#include "hash_table.h" // hash_table, node
#include "../macros.h" // Free

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

static unsigned long get_index(char const *key, size_t size);

hash_table new_table(size_t nmemb)
{
    return vec_alloc(nmemb * sizeof (node *));
}

void delete_node(char const *k, hash_table t)
{
    node *crawler = t[get_index(k, vec_capacity(t) / sizeof *t)];
    node *prev = NULL;
    while (crawler) {
        if (strcmp(k, crawler->key) == 0) {
            if (prev) {
                prev->next = crawler->next;
            }
            Free(crawler);
            return;
        }
    }
}

int add_node(char const *k, void *v, hash_table t)
{
    if (!t) {
        return -1;
    }

    node *new = malloc(sizeof (node));
    Assert_alloc(new);

    new->key = k;
    new->value = v;
    unsigned long i = get_index(k, vec_capacity(t) / sizeof *t);
    new->next = t[i];

    t[i] = new;
    return 0;
}

void *find_node(char const *k, _Bool (*filter)(void *), hash_table t)
{
    if (!t) {
        return NULL;
    }
    node *crawler = t[get_index(k, vec_capacity(t) / sizeof *t)];
    while (crawler) {
        if (strcmp(crawler->key, k) == 0) {
            _Bool end = filter ? filter(crawler->value) : 1;
            if (end) {
                return crawler->value;
            } else {
                continue;
            }
        }
        crawler = crawler->next;
    }
    return NULL;
}

void free_table(hash_table t, void (*destructor)(node *))
{
    if (!t) {
        return;
    }
    size_t table_cap = vec_capacity(t) / sizeof *t;
    // Free contents
    for (size_t i = 0; i < table_cap; i++) {
        node *crawler = t[i];
        while (crawler) {
            if (destructor) {
                destructor(crawler);
            }
            node *next = crawler->next;
            Free(crawler);
            crawler = next;
        }
    }
    vec_free(t);
}


// Modified djb2
// Requires key to be a valid string ending in '\0'
static unsigned long get_index(char const *key, size_t size)
{
    unsigned long hash = 5381;
    while (*key++) {
        hash = ((hash << 5) + hash) + *key;
    }
    return hash & (size - 1);
}
