#include <stdlib.h> // malloc, realloc
#include <string.h> // strcmp

#include "../helpers.h" // grow_array
#include "hash_table.h" // hash_table, node
#include "../macros.h" // Free

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

#define TABLE_GROWTH_FACTOR 2
static unsigned long get_index(char const *key, size_t size);

hash_table *new_table(size_t size)
{
    if (size == 0) {
        return NULL;
    }
    hash_table *ret = malloc(sizeof *ret);
    if (!ret) {
        return NULL;
    }
    ret->nodes = calloc(size, sizeof (node*));
    if (!ret->nodes) {
        free(ret);
        return NULL;
    }
    ret->size = 0;
    ret->capacity = size;
    return ret;
}

void delete_node(char const *k, hash_table *t)
{
    node *crawler = t->nodes[get_index(k, t->capacity)];
    node *prev = NULL;
    while (crawler) {
        if (strcmp(k, crawler->key) == 0) {
            if (prev) {
                prev->next = crawler->next;
            }
            Free(crawler);
            t->size--;
            return;
        }
    }
}

int add_node(char const *k, void *v, hash_table *t)
{
    if (!t || !t->nodes) {
        return -1;
    }
    if (t->size + 1 >= t->capacity) {
        t->nodes = grow_array(t->nodes, &t->capacity);
        if (!t->nodes) {
            return -1;
        }
    }

    node *new = malloc(sizeof (node));
    if (!new) {
        return -1;
    }

    new->key = k;
    new->value = v;
    unsigned long i = get_index(k, t->capacity);
    new->next = t->nodes[i];

    t->nodes[i] = new;
    t->size++;
    return 0;
}

void *find_node(char const *k, hash_table const *t)
{
    if (!t || !t->nodes) {
        return NULL;
    }
    node *crawler = t->nodes[get_index(k, t->capacity)];
    while (crawler) {
        if (strcmp(crawler->key, k) == 0) {
            return crawler->value;
        }
        crawler = crawler->next;
    }
    return NULL;
}

void free_table(hash_table **t)
{
    if (!t) {
        return;
    }
    for (size_t i = 0; i < (*t)->capacity; i++) {
        node *crawler = (*t)->nodes[i];
        while (crawler) {
            node *next = crawler->next;
            Free(crawler);
            crawler = next;
        }
    }
    Free((*t)->nodes);
    Free(*t);
}


// Modified djb2
static unsigned long get_index(char const *key, size_t size)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash & (size - 1);
}
