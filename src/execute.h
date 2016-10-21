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

#ifndef MARCEL_EXEC_H
#define MARCEL_EXEC_H

#include "ds/hash_table.h"
#include "ds/proc.h" // proc

// Builtin function
typedef int (*proc_func)(proc const*);

int launch_job(job *j);
_Bool initialize_builtins(void);


// Tagged pointers would be a nice optimization but they don't seemt to work with function pointers
typedef struct builtin {
    union {
        proc_func cmd;
        char *var;
    };
    int type;
} builtin;

enum {
    CMD,
    VAR,
};

extern hash_table lookup_table;

#endif
