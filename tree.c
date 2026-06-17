#define _POSIX_C_SOURCE 200809L  /* must precede all includes: exposes strdup under -std=c11 -pedantic */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>               /* for isalnum() used in validation */

#include "tree.h"
#include "node.h"

/* ---------- registry (internal to this file) ---------- */

/* One entry per distinct token, used only during Pass 1 to accumulate counts
   before any tree node exists. Kept file-local since it is an implementation
   detail of buildTree and is never exposed to callers. */
typedef struct {
    char *token;     /* strdup copy of the word */
    int   frequency; /* running count of how many times it has appeared */
} StringEntry;

/* Return 1 if every character of s is a letter or digit, 0 otherwise.
   Used to validate input tokens (the +10% extra-credit requirement). */
static int is_alphanumeric(const char *s)
{
    for (; *s; s++)
        /* Cast to unsigned char: isalnum has UB for negative (signed) char values. */
        if (!isalnum((unsigned char)*s))
            return 0;             /* found a non-alphanumeric char -> reject */
    return 1;                     /* all characters were alphanumeric */
}

/* ---------- node helpers ---------- */

/* Append a copy of string s to node n's strings array, growing it if full. */
static void node_add_string(Node *n, const char *s)
{
    if (n->str_count == n->str_cap) {
        /* Out of room: start at 4 slots, then double on each reallocation. */
        n->str_cap = n->str_cap ? n->str_cap * 2 : 4;
        n->strings = realloc(n->strings, n->str_cap * sizeof(char *));
        if (!n->strings) {
            fprintf(stderr, "Error: out of memory\n");
            exit(1);
        }
    }
    n->strings[n->str_count++] = strdup(s);  /* node owns its own copy of the word */
}

/* Allocate a new node for the given frequency, seeded with its first string. */
static Node *make_node(int frequency, const char *first_string)
{
    Node *n = calloc(1, sizeof(Node));       /* calloc zeroes counts and child pointers */
    if (!n) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }
    n->frequency = frequency;
    node_add_string(n, first_string);        /* allocates the strings array (NULL -> 4 slots) */
    return n;
}

/* ---------- BST helpers ---------- */

/* Search the tree for a node whose key equals `frequency`; return it or NULL.
   Iterative descent: equal = found, smaller = go left, larger = go right. */
static Node *bst_find(Node *root, int frequency)
{
    while (root) {
        if (frequency == root->frequency)
            return root;
        root = (frequency < root->frequency) ? root->left : root->right;
    }
    return NULL;                  /* no node with that frequency exists yet */
}

/* Insert new_node into the BST by frequency and return the (possibly new) root.
   Frequencies are finalized before the BST is built, so bst_find always
   handles the equal-frequency case; this function never sees a duplicate key. */
static Node *bst_insert(Node *root, Node *new_node)
{
    if (!root)                                       /* empty spot found: place node here */
        return new_node;
    if (new_node->frequency < root->frequency)
        root->left  = bst_insert(root->left,  new_node);   /* smaller -> left subtree  */
    else
        root->right = bst_insert(root->right, new_node);   /* larger  -> right subtree */
    return root;                                     /* root unchanged; subtree updated */
}

/* ---------- print helper ---------- */

/* Write one node as: <2*depth spaces><frequency>:<space-separated words><newline>. */
static void print_node(Node *n, int depth, FILE *out)
{
    int i;
    for (i = 0; i < 2 * depth; i++)        /* indentation: two spaces per tree level */
        fputc(' ', out);
    fprintf(out, "%d:", n->frequency);     /* the node's frequency, then a literal colon */
    for (i = 0; i < n->str_count; i++)
        fprintf(out, " %s", n->strings[i]);/* each word, space-prefixed, in first-appearance order */
    fputc('\n', out);
}

/* ---------- public API ---------- */

/* Read all tokens from `in` and return the root of a frequency-ordered BST. */
Node *buildTree(FILE *in)
{
    StringEntry *registry = NULL;          /* dynamic array of distinct tokens + counts */
    int          reg_count = 0, reg_cap = 0;
    char         buf[1024];                /* holds one token (up to 1023 chars + '\0') */
    Node        *root = NULL;
    int          i;

    /* --- Pass 1: read all tokens, accumulate frequencies in the registry --- */
    /* "%1023s" skips whitespace and reads one token; returns 1 each time, EOF at end. */
    while (fscanf(in, "%1023s", buf) == 1) {
        if (!is_alphanumeric(buf)) {       /* +10%: warn on and skip any bad token */
            fprintf(stderr, "Warning: skipping non-alphanumeric token '%s'\n", buf);
            continue;
        }
        /* Linear search for an existing entry with this exact token. */
        for (i = 0; i < reg_count; i++) {
            if (strcmp(registry[i].token, buf) == 0) {
                registry[i].frequency++;   /* seen before: just bump the count */
                break;
            }
        }
        /* If the loop ran to the end without breaking, i == reg_count: a new token. */
        if (i == reg_count) {
            if (reg_count == reg_cap) {    /* grow the registry when full (size unknown) */
                reg_cap = reg_cap ? reg_cap * 2 : 16;   /* start at 16, then double */
                registry = realloc(registry, reg_cap * sizeof(StringEntry));
                if (!registry) {
                    fprintf(stderr, "Error: out of memory\n");
                    exit(1);
                }
            }
            registry[reg_count].token     = strdup(buf);  /* own a copy of the word */
            registry[reg_count].frequency = 1;            /* first sighting */
            reg_count++;
        }
    }

    /* --- Pass 2: build the BST from the registry in first-appearance order --- */
    for (i = 0; i < reg_count; i++) {
        Node *existing = bst_find(root, registry[i].frequency);
        if (existing) {
            /* A node with this frequency already exists: add the word to it so
               equal-frequency tokens share one node, in first-appearance order. */
            node_add_string(existing, registry[i].token);
        } else {
            /* First token at this frequency: make a node and insert it. */
            Node *new_node = make_node(registry[i].frequency, registry[i].token);
            root = bst_insert(root, new_node);
        }
        free(registry[i].token);           /* registry copy no longer needed */
    }
    free(registry);                        /* free the registry array itself */

    return root;                           /* NULL if the input was empty */
}

/* Preorder: print the node, then its left subtree, then its right subtree. */
void printPreorder(Node *root, int depth, FILE *out)
{
    if (!root) return;                     /* base case: empty subtree prints nothing */
    print_node(root, depth, out);
    printPreorder(root->left,  depth + 1, out);  /* children are one level deeper */
    printPreorder(root->right, depth + 1, out);
}

/* Inorder: left subtree, then the node, then the right subtree. */
void printInorder(Node *root, int depth, FILE *out)
{
    if (!root) return;
    printInorder(root->left,  depth + 1, out);
    print_node(root, depth, out);
    printInorder(root->right, depth + 1, out);
}

/* Postorder: both subtrees first, then the node. */
void printPostorder(Node *root, int depth, FILE *out)
{
    if (!root) return;
    printPostorder(root->left,  depth + 1, out);
    printPostorder(root->right, depth + 1, out);
    print_node(root, depth, out);
}

/* Free the whole tree: children first (postorder), so no node is used after free. */
void free_tree(Node *root)
{
    int i;
    if (!root) return;                     /* base case */
    free_tree(root->left);
    free_tree(root->right);
    for (i = 0; i < root->str_count; i++)
        free(root->strings[i]);            /* free each strdup'd word */
    free(root->strings);                   /* free the array of pointers */
    free(root);                            /* free the node itself */
}
