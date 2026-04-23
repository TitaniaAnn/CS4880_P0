#ifndef NODE_H
#define NODE_H

typedef struct Node {
    int          frequency; /* BST key: number of times any string in this node appeared */
    char       **strings;   /* dynamic array of strings, in order of first appearance */
    int          str_count; /* number of strings stored */
    int          str_cap;   /* allocated capacity of strings array */
    struct Node *left;
    struct Node *right;
} Node;

#endif /* NODE_H */
