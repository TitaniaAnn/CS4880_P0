#ifndef NODE_H              /* include guard: prevents double inclusion */
#define NODE_H

/* A single BST node. The tree is keyed by `frequency`; every distinct word that
   appeared the same number of times is stored together in this one node's
   `strings` array (in the order the words were first seen in the input). */
typedef struct Node {
    int          frequency; /* BST key: number of times any string in this node appeared */
    char       **strings;   /* dynamic array of strings, in order of first appearance */
    int          str_count; /* number of strings currently stored */
    int          str_cap;   /* allocated capacity of the strings array (>= str_count) */
    struct Node *left;      /* subtree of strictly smaller frequencies */
    struct Node *right;     /* subtree of strictly larger frequencies */
} Node;

#endif /* NODE_H */
