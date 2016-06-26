#include <stdlib.h> // malloc, realloc
#include <string.h> // strcmp
#include "hash_table.h" // hash_table, node
#include "msh_macros.h" // Stopif, Free

static unsigned long get_index(char const *key, size_t size);

int add_node(void *v, char const *k, hash_table *t)
{
    if (!t || !t->nodes) {
        return -1;
    }

    if (t->size + 1 >= t->capacity) {
        node **tmp = realloc(t->nodes, t->capacity * GROWTH_FACTOR);
        if (!tmp) return -1;
        t->nodes = tmp;
    }

    node *new = malloc(sizeof (node));
    if (!new) {
        return -1;
    }

    new->key = k;
    new->value = v;
    unsigned long i = get_index(k, t->capacity);
    new->next = t->nodes[i];

    t->nodes[i] = new;
    t->size++;
    return 0;
}

void *find_node(char const *k, hash_table const *t)
{
    if (!t || !t->nodes) {
        return NULL;
    }
    node *crawler = t->nodes[get_index(k, t->capacity)];
    while (crawler) {
        if (strcmp(crawler->key, k) == 0) {
            return crawler->value;
        }
        crawler = crawler->next;
    }
    return NULL;
}

void free_table(hash_table *t)
{
    if (!t) {
        return;
    }
    for (size_t i = 0; i < t->capacity; i++) {
        node *crawler = t->nodes[i];
        while (crawler) {
            node *next = crawler->next;
            Free(crawler);
            crawler = next;
        }
    }
    Free(t->nodes);
}

// Modified djb2
static unsigned long get_index(char const *key, size_t size)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash & (size - 1);
}
