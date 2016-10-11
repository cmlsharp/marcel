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
    return new_vec(nmemb, sizeof (node *));
}

void delete_node(char const *k, hash_table *t)
{
    node **nodes = (node **)t->data;
    node *crawler = nodes[get_index(k, t->cap)];
    node *prev = NULL;
    while (crawler) {
        if (strcmp(k, crawler->key) == 0) {
            if (prev) {
                prev->next = crawler->next;
            }
            Free(crawler);
            t->num--;
            return;
        }
    }
}

int add_node(char const *k, void *v, hash_table *t)
{
    if (!t || !t->data) {
        return -1;
    }

    node *new = malloc(sizeof (node));
    Assert_alloc(new);

    new->key = k;
    new->value = v;
    unsigned long i = get_index(k, t->cap);
    node **nodes = (node **) t->data;
    new->next = nodes[i];

    nodes[i] = new;
    t->num++;
    return 0;
}

void *find_node(char const *k, hash_table const *t)
{
    if (!t || !t->data) {
        return NULL;
    }
    node **nodes = (node **) t->data;
    node *crawler = nodes[get_index(k, t->cap)];
    while (crawler) {
        if (strcmp(crawler->key, k) == 0) {
            return crawler->value;
        }
        crawler = crawler->next;
    }
    return NULL;
}

void free_table(hash_table *t)
{
    if (!t) {
        return;
    }
    // Free contents
    node **nodes = (node **) t->data;
    for (size_t i = 0; i < t->cap; i++) {
        node *crawler = nodes[i];
        while (crawler) {
            node *next = crawler->next;
            Free(crawler);
            crawler = next;
        }
    }
    free_vec(t);
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
