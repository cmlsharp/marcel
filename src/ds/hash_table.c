#include <stdlib.h> // malloc, realloc
#include <string.h> // strcmp

#include "hash_table.h" // hash_table, node
#include "../macros.h" // Free

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

#define TABLE_GROWTH_FACTOR 2
static unsigned long get_index(char const *key, size_t size);

__attribute__((always_inline))
inline hash_table *new_table(size_t nmemb)
{
    return new_dyn_array(nmemb, sizeof (node *));
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
    if (t->num + 1 >= t->cap) {
        if (grow_dyn_array(t) != 0) {
            return -1;
        }
    }
    node *new = malloc(sizeof (node));
    if (!new) {
        return -1;
    }

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
    free_dyn_array(t);
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
