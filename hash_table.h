#ifndef MSH_HASH_H
#define MSH_HASH_H

#include "msh_execute.h"

#define TABLE_SIZE 64

typedef struct node {
    cmd_func f;
    char const* s;
    struct node *next;
} node;

int add_node(cmd_func f, char const *s, node **table);
cmd_func find_node(char const *s, node **table);
void free_table(node **table);

#endif
