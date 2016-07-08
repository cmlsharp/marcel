#ifndef MARCEL_CHILDREN_H
#define MARCEL_CHILDREN_H

#include "marcel.h" // _Bool typedef for g++ compatibility
#include <sys/types.h> // pid_t

#define MAX_BKG_CHILD 1024

size_t add_bkg_child(pid_t p);
size_t del_bkg_child(pid_t p);
size_t num_bkg_child(void);

void set_active_child(pid_t p);
void reset_active_child(void);

pid_t get_active_child(void);
_Bool has_active_child(void);

#endif
