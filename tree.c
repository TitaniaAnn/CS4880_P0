#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tree.h"
#include "node.h"

/* ---------- registry (internal to this file) ---------- */

/* One registry slot per distinct token seen during pass 1. The registry is
   the "container" of (string, frequency) pairs the assignment calls for. */
typedef struct {
    char *token;     /* strdup copy of the token text (owned by the registry) */
    int   frequency; /* how many times this token appeared in the input */
} StringEntry;

/* Return 1 if every character of s is a letter or digit, 0 otherwise.
   Used to validate tokens; the empty string is treated as alphanumeric. */
static int is_alphanumeric(const char *s)
{
    for (; *s; s++)
        /* Cast to unsigned char: isalnum has UB for negative (signed) char values. */
        if (!isalnum((unsigned char)*s))
            return 0;
    return 1;
}

/* ---------- node helpers ---------- */

/* Append a copy of string s to node n's dynamic strings[] array, growing it
   if necessary. Every token sharing n's frequency is stored here. */
static void node_add_string(Node *n, const char *s)
{
    if (n->str_count == n->str_cap) {
        /* Array is full: start at 4 slots, then double on each reallocation. */
        n->str_cap = n->str_cap ? n->str_cap * 2 : 4;
        n->strings = realloc(n->strings, n->str_cap * sizeof(char *));
        if (!n->strings) {
            fprintf(stderr, "Error: out of memory\n");
            exit(1);
        }
    }
    /* strdup so the node owns its own copy, independent of the registry. */
    n->strings[n->str_count++] = strdup(s);
}

/* Allocate a new leaf node for the given frequency, seeded with one string.
   calloc zeroes the struct so left/right/strings/counts all start empty. */
static Node *make_node(int frequency, const char *first_string)
{
    Node *n = calloc(1, sizeof(Node));
    if (!n) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }
    n->frequency = frequency;
    node_add_string(n, first_string);   /* allocates strings[] on first use */
    return n;
}

/* ---------- BST helpers ---------- */

/* Iterative BST search by frequency key. Returns the matching node, or NULL.
   Smaller frequencies live left, larger ones right. */
static Node *bst_find(Node *root, int frequency)
{
    while (root) {
        if (frequency == root->frequency)
            return root;                              /* found the node */
        /* Descend toward the side that could hold the key. */
        root = (frequency < root->frequency) ? root->left : root->right;
    }
    return NULL;                                      /* no such frequency */
}

/* Frequencies are finalized before the BST is built, so bst_find always
   handles the equal-frequency case; this function never sees a duplicate key. */
static Node *bst_insert(Node *root, Node *new_node)
{
    if (!root)
        return new_node;                              /* empty spot: place it here */
    if (new_node->frequency < root->frequency)
        root->left  = bst_insert(root->left,  new_node);   /* go left */
    else
        root->right = bst_insert(root->right, new_node);   /* go right (key is unique) */
    return root;                                      /* unchanged subtree root */
}

/* ---------- print helper ---------- */

/* Print one node in the required format:
   "<2*depth spaces><frequency>: <token1> <token2> ...\n" */
static void print_node(Node *n, int depth, FILE *out)
{
    int i;
    for (i = 0; i < 2 * depth; i++)   /* indent two spaces per level of depth */
        fputc(' ', out);
    fprintf(out, "%d:", n->frequency);
    for (i = 0; i < n->str_count; i++)
        fprintf(out, " %s", n->strings[i]);   /* leading space separates each token */
    fputc('\n', out);
}

/* ---------- public API ---------- */

Node *buildTree(FILE *in)
{
    StringEntry *registry = NULL;
    int          reg_count = 0, reg_cap = 0;
    char         buf[1024];
    Node        *root = NULL;
    int          i;

    /* Pass 1: read every whitespace-delimited token and tally frequencies.
       The frequency is the BST key, but it isn't known until all input is
       seen, so we count here and build the tree afterwards. */
    while (fscanf(in, "%1023s", buf) == 1) {   /* %1023s guards against overflow */
        /* Data validation: only letters/digits are valid; warn and skip others. */
        if (!is_alphanumeric(buf)) {
            fprintf(stderr, "Warning: skipping non-alphanumeric token '%s'\n", buf);
            continue;
        }
        /* Look for this token already in the registry (linear scan). */
        for (i = 0; i < reg_count; i++) {
            if (strcmp(registry[i].token, buf) == 0) {
                registry[i].frequency++;   /* seen before: bump its count */
                break;
            }
        }
        /* If the loop exhausted without a match, i == reg_count: new token. */
        if (i == reg_count) {
            if (reg_count == reg_cap) {
                /* Grow the registry: start at 16 slots, then double. */
                reg_cap = reg_cap ? reg_cap * 2 : 16;
                registry = realloc(registry, reg_cap * sizeof(StringEntry));
                if (!registry) {
                    fprintf(stderr, "Error: out of memory\n");
                    exit(1);
                }
            }
            /* Record the new token in first-appearance order, frequency 1. */
            registry[reg_count].token     = strdup(buf);
            registry[reg_count].frequency = 1;
            reg_count++;
        }
    }

    /* Pass 2: build the BST from the registry in first-appearance order, so
       tokens are created using their first occurrence and never rearranged. */
    for (i = 0; i < reg_count; i++) {
        Node *existing = bst_find(root, registry[i].frequency);
        if (existing) {
            /* A node with this frequency already exists: equal-frequency
               tokens are "equivalent", so they share that node's list. */
            node_add_string(existing, registry[i].token);
        } else {
            /* New frequency: create a node and insert it by key. */
            Node *new_node = make_node(registry[i].frequency, registry[i].token);
            root = bst_insert(root, new_node);
        }
        free(registry[i].token);   /* node kept its own copy; free the registry's */
    }
    free(registry);                /* registry slots are done with */

    return root;
}

/* Preorder: node, then left subtree, then right subtree. */
void printPreorder(Node *root, int depth, FILE *out)
{
    if (!root) return;                          /* empty subtree prints nothing */
    print_node(root, depth, out);               /* root first */
    printPreorder(root->left,  depth + 1, out);
    printPreorder(root->right, depth + 1, out);
}

/* Inorder: left subtree, then node, then right subtree. */
void printInorder(Node *root, int depth, FILE *out)
{
    if (!root) return;
    printInorder(root->left,  depth + 1, out);
    print_node(root, depth, out);               /* root between the children */
    printInorder(root->right, depth + 1, out);
}

/* Postorder: left subtree, then right subtree, then node. */
void printPostorder(Node *root, int depth, FILE *out)
{
    if (!root) return;
    printPostorder(root->left,  depth + 1, out);
    printPostorder(root->right, depth + 1, out);
    print_node(root, depth, out);               /* root last */
}

/* Postorder free: release both subtrees before the node itself, so no node
   is freed while it still has live children to recurse into. */
void free_tree(Node *root)
{
    int i;
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    for (i = 0; i < root->str_count; i++)
        free(root->strings[i]);   /* each strdup'd token */
    free(root->strings);          /* the array of pointers */
    free(root);                   /* finally the node */
}
