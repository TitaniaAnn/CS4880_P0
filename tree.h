#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include "node.h"

/* Read all tokens from in, build and return the root of a frequency-ordered BST.
   Two passes: first collects tokens and frequencies, second constructs the tree. */
Node *buildTree(FILE *in);

/* Print traversals to the given open FILE, starting at depth 0. */
void printPreorder (Node *root, int depth, FILE *out);
void printInorder  (Node *root, int depth, FILE *out);
void printPostorder(Node *root, int depth, FILE *out);

/* Recursively free all nodes and their string arrays. */
void free_tree(Node *root);

#endif /* TREE_H */
