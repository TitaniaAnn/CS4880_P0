#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include "node.h"

/* One entry per unique token: tracks frequency and first-appearance order */
typedef struct {
    char *token;     /* strdup copy owned by the registry */
    int   frequency;
    int   first_idx; /* index into registry at time of first insertion */
} StringEntry;

/* Build a BST from the registry (two-pass: registry already has final frequencies).
   Returns the root of the constructed BST. */
Node *buildTree(StringEntry *registry, int count);

/* Print traversals to the given open FILE, starting at depth 0. */
void printPreorder (Node *root, int depth, FILE *out);
void printInorder  (Node *root, int depth, FILE *out);
void printPostorder(Node *root, int depth, FILE *out);

/* Recursively free all nodes and their string arrays. */
void free_tree(Node *root);

#endif /* TREE_H */
