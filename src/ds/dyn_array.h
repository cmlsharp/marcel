#ifndef M_DYN_H
#define M_DYN_H
typedef struct dyn_array {
    void *data;
    size_t cap; // Allocated size
    size_t num; // Number of used indicies
    size_t size; // Size of each member of array
} dyn_array;

dyn_array *new_dyn_array(size_t nmemb, size_t size);
void free_dyn_array(dyn_array *d);
int grow_dyn_array(dyn_array *d);
#endif
