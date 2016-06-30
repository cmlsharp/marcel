#include "children.h" // pid_t


typedef struct child_rec {
    pid_t active;
    pid_t bkg[MAX_BKG_PROC];
    size_t count;
} child_rec;

child_rec volatile c_rec;

// Adds process to record and returns its job number. 0 indicates no available
// records.
size_t add_bkg_proc(pid_t p)
{
    size_t i;
    for (i = 0; c_rec.bkg[i]; i++) {
        if (i == MAX_BKG_PROC - 1) return 0;
    }
    c_rec.bkg[i] = p;
    c_rec.count++;
    return i+1;
}

// Removes process from record and returns its former job number. 0 indicates
// no process found.
size_t del_bkg_proc(pid_t p)
{
    size_t i;
    for (i = 0; i < MAX_BKG_PROC; i++) {
        if (c_rec.bkg[i] == p) {
            c_rec.bkg[i] = 0;
            return i + 1;
        }
    }
    c_rec.count--;
    return 0;
}

__attribute__((always_inline))
inline size_t num_bkg_proc(void)
{
    return c_rec.count;
}
__attribute__((always_inline))
inline void set_active_child(pid_t p)
{
    c_rec.active = p;
}

__attribute__((always_inline))
inline void reset_active_child(void)
{
    set_active_child(0);
}

__attribute__((always_inline))
inline pid_t get_active_child(void)
{
    return c_rec.active;
}

__attribute__((always_inline))
inline _Bool has_active_child(void)
{
    return !!c_rec.active;
}
