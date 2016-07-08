#include <stdlib.h>

#include "cmd.h" // cmd
#include "../macros.h" // Cast, Assert_alloc, Free

cmd *new_cmd(void)
{
    cmd *ret = Cast(cmd*) calloc(1, sizeof *ret);
    Assert_alloc(ret);
    ret->argv = new_dyn_array(ARGV_INIT_SIZE, sizeof (char*));
    ret->env = new_dyn_array(ARGV_INIT_SIZE, sizeof (char*));

    for (size_t i = 0; i < Arr_len(ret->fds); i++) {
        ret->fds[i] = i;
    }

    ret->wait = 1;
    return ret;
}

// Frees cmd and dynamically allocated members.
// TODO: Make less ugly.
void free_cmd(cmd *c)
{
    while (c) {
        dyn_array **a[] = {&c->argv, &c->env};
        for (size_t i = 0 ; i < Arr_len(a); i++) {
            char ***strs = (char ***) &(*a[i])->data;
            for (size_t j = 0; j < (*a[i])->cap && (*strs)[j] ; j++) {
                Free((*strs)[j]);
            }
            free_dyn_array(*a[i]);
        }

        cmd *next = c->next;
        Free(c);
        c = next;
    }
}

cmd_wrapper *new_cmd_wrapper(void)
{
    cmd_wrapper *ret = Cast(cmd_wrapper*) calloc(1, sizeof *ret);
    Assert_alloc(ret);
    return ret;
}

void free_cmd_wrapper(cmd_wrapper *w)
{
    if (!w) return;
    for (size_t i = 0; i < Arr_len(w->io); i++) {
        Free(w->io[i].path);
    }
    free_cmd(w->root);
    Free(w);
}

