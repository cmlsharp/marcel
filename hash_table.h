#ifndef MSH_HASH_H
#define MSH_HASH_H

#include "msh_execute.h"

#define TABLE_SIZE 64

// Hashtable that maps names to the shell's builtin functions. A hashtable is
// overkill of course with so few builtin functions, but it was fun to implement
typedef struct node {
    cmd_func f; // Pointer to builtin func
    char const* s; // Name of builtin func
    struct node *next;
} node;

int add_node(cmd_func f, char const *s, node **table);
cmd_func find_node(char const *s, node **table);
void free_table(node **table);

#endif
