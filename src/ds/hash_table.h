#ifndef MARCEL_HASH_H
#define MARCEL_HASH_H

#define TABLE_INIT_SIZE 1024

#include "dyn_array.h"


typedef struct node {
    void *value; // Pointer to builtin function (or alias)
    char const* key; // Name of function (or alias)
    struct node *next; // Pointer to next node (in case of conflicts
} node;

typedef dyn_array hash_table;

hash_table *new_table(size_t size);
int add_node(char const *k, void *v, hash_table *t);
void *find_node(char const *k, hash_table const *t);
void delete_node(char const *k, hash_table *t);
void free_table(hash_table *t);

#endif
