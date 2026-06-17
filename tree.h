#ifndef TREE_H              /* include guard: prevents double inclusion */
#define TREE_H

#include <stdio.h>          /* for FILE used in the prototypes below */
#include "node.h"           /* for the Node type returned/traversed here */

/* Read all tokens from `in`, build and return the root of a frequency-ordered BST.
   Two passes: first collects tokens and frequencies, second constructs the tree. */
Node *buildTree(FILE *in);

/* Print one traversal to the already-open file `out`. `depth` is the caller's
   starting depth (always 0 for the root) and controls the leading indentation. */
void printPreorder (Node *root, int depth, FILE *out);  /* root, left, right  */
void printInorder  (Node *root, int depth, FILE *out);  /* left, root, right  */
void printPostorder(Node *root, int depth, FILE *out);  /* left, right, root  */

/* Recursively free every node and its string array. Safe to call on NULL. */
void free_tree(Node *root);

#endif /* TREE_H */
