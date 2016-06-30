#ifndef MSH_CHILDREN_H
#define MSH_CHILDREN_H

#include <sys/types.h> // pid_t

#define MAX_BKG_PROC 1024

size_t add_bkg_proc(pid_t p);
size_t del_bkg_proc(pid_t p);
size_t num_bkg_proc(void);

void set_active_child(pid_t p);
void reset_active_child(void);

pid_t get_active_child(void);
_Bool has_active_child(void);

#endif
