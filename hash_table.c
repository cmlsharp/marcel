#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

static unsigned long hash(char const *str);

int add_node(cmd_func f, char const *s, node **table)
{
    if (!table) return -1;
    unsigned long i = hash(s) & (TABLE_SIZE - 1); // Mod
    node *crawler = NULL;

    node *new = malloc(sizeof (node));
    if (!new) return -1;

    if (!table[i]) {
        table[i] = new;
        crawler = table[i];
    } else {
        crawler = table[i];
        while (crawler->next) crawler = crawler->next;
        crawler->next = new;
        crawler = crawler->next;
    }
    crawler->f = f;
    crawler->s = s;
    crawler->next = NULL;
    return 0;
}

cmd_func find_node(char const *s, node **table)
{
    if (!table) return NULL;    
    node *crawler = table[hash(s) & (TABLE_SIZE-1)];
    while (crawler) {
        if (strcmp(crawler->s, s) == 0) return crawler->f;
        crawler = crawler->next;
    }
    return NULL;
}

void free_table(node **table)
{
    if (!table) return;
    for (size_t i = 0; i < TABLE_SIZE; i++) {
        node *crawler = table[i];
        while (crawler) {
            node *next = crawler->next;
            free(crawler);
            crawler = next;
        }
    }
}

// djb2
static unsigned long hash(char const *str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}
