#include <stdlib.h>

#include "proc.h"
#include "../macros.h"

proc *new_proc(void)
{
    proc *ret = calloc(1, sizeof *ret);
    Assert_alloc(ret);
    ret->argv = new_dyn_array(ARGV_INIT_SIZE, sizeof (char*));
    ret->env = new_dyn_array(ARGV_INIT_SIZE, sizeof (char*));

    for (size_t i = 0; i < Arr_len(ret->fds); i++) {
        ret->fds[i] = i;
    }
    return ret;
}

// Frees proc and dynamically allocated members.
// TODO: Make less ugly.
void free_proc(proc *p)
{
    while (p) {
        dyn_array **a[] = {&p->argv, &p->env};
        for (size_t i = 0 ; i < Arr_len(a); i++) {
            char ***strs = (char ***) &(*a[i])->data;
            for (size_t j = 0; j < (*a[i])->cap && (*strs)[j] ; j++) {
                Free((*strs)[j]);
            }
            free_dyn_array(*a[i]);
        }

        proc *next = p->next;
        Free(p);
        p = next;
    }
}

// Allocate new job with all fields initialized to 0. Panics on allocation
// failure
job *new_job(void)
{
    job *ret = calloc(1, sizeof *ret);
    Assert_alloc(ret);
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
    free_proc(j->root);
    Free(j);
}

