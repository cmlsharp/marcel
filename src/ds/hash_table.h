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

#ifndef MARCEL_HASH_H
#define MARCEL_HASH_H

#define TABLE_INIT_SIZE 1024

#include "vec.h"


typedef struct node {
    void *value; // Pointer to builtin function (or alias)
    char const* key; // Name of function (or alias)
    struct node *next; // Pointer to next node (in case of conflicts)
} node;

typedef node** hash_table;

hash_table new_table(size_t size);
int add_node(char const *k, void *v, hash_table t);
void *find_node(char const *k, _Bool (*filter)(void *), hash_table t);
void delete_node(char const *k, hash_table t);
void free_table(hash_table t, void (*destructor)(node*));

#endif
