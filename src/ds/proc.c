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

#include <stdlib.h>

#include "proc.h"
#include "../macros.h"
#define INITIAL_PROC_CAP 4

proc *new_proc(void)
{
    proc *ret = calloc(1, sizeof *ret);
    Assert_alloc(ret);
    ret->argv = vec_alloc(ARGV_INIT_SIZE * sizeof *ret->argv);
    ret->env =  vec_alloc(ARGV_INIT_SIZE * sizeof *ret->env);

    for (size_t i = 0; i < Arr_len(ret->fds); i++) {
        ret->fds[i] = i;
    }
    return ret;
}

// Frees proc and dynamically allocated members.
// TODO: Make less ugly.
void free_proc(proc *p)
{
    char ***a[] = {&p->argv, &p->env};
    for (size_t i = 0 ; i < Arr_len(a); i++) {
        size_t len = vec_len(*a[i]);
        for (size_t j = 0; j < len; j++) {
            Free((*a[i])[j]);
        }
        vec_free(*a[i]);
    }
    Free(p);
}

// Allocate new job with all fields initialized to 0. Panics on allocation
// failure
job *new_job(void)
{
    job *ret = calloc(1, sizeof *ret);
    Assert_alloc(ret);
    ret->procs = vec_alloc(INITIAL_PROC_CAP * sizeof *ret->procs);
    return ret;
}

// Free all dynamically allocated fields in job and job itself
void free_single_job(job *j)
{
    if (!j) {
        return;
    }
    for (size_t i = 0; i < Arr_len(j->io); i++) {
        Free(j->io[i].path);
    }
    Free(j->name);
    size_t proc_len = vec_len(j->procs);
    for (proc **p_p = j->procs; p_p < j->procs + proc_len; p_p++) {
        Cleanup(*p_p, free_proc);
    }
    vec_free(j->procs);
    Free(j);
}

