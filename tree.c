#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tree.h"
#include "node.h"

/* ---------- registry (internal to this file) ---------- */

typedef struct {
    char *token;     /* strdup copy */
    int   frequency;
} StringEntry;

static int is_alphanumeric(const char *s)
{
    for (; *s; s++)
        /* Cast to unsigned char: isalnum has UB for negative (signed) char values. */
        if (!isalnum((unsigned char)*s))
            return 0;
    return 1;
}

/* ---------- node helpers ---------- */

static void node_add_string(Node *n, const char *s)
{
    if (n->str_count == n->str_cap) {
        /* Start at 4 slots; double on each reallocation. */
        n->str_cap = n->str_cap ? n->str_cap * 2 : 4;
        n->strings = realloc(n->strings, n->str_cap * sizeof(char *));
        if (!n->strings) {
            fprintf(stderr, "Error: out of memory\n");
            exit(1);
        }
    }
    n->strings[n->str_count++] = strdup(s);
}

static Node *make_node(int frequency, const char *first_string)
{
    Node *n = calloc(1, sizeof(Node));
    if (!n) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }
    n->frequency = frequency;
    node_add_string(n, first_string);
    return n;
}

/* ---------- BST helpers ---------- */

static Node *bst_find(Node *root, int frequency)
{
    while (root) {
        if (frequency == root->frequency)
            return root;
        root = (frequency < root->frequency) ? root->left : root->right;
    }
    return NULL;
}

/* Frequencies are finalized before the BST is built, so bst_find always
   handles the equal-frequency case; this function never sees a duplicate key. */
static Node *bst_insert(Node *root, Node *new_node)
{
    if (!root)
        return new_node;
    if (new_node->frequency < root->frequency)
        root->left  = bst_insert(root->left,  new_node);
    else
        root->right = bst_insert(root->right, new_node);
    return root;
}

/* ---------- print helper ---------- */

static void print_node(Node *n, int depth, FILE *out)
{
    int i;
    for (i = 0; i < 2 * depth; i++)
        fputc(' ', out);
    fprintf(out, "%d:", n->frequency);
    for (i = 0; i < n->str_count; i++)
        fprintf(out, " %s", n->strings[i]);
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

    /* Pass 1: read all tokens, accumulate frequencies in registry */
    while (fscanf(in, "%1023s", buf) == 1) {
        if (!is_alphanumeric(buf)) {
            fprintf(stderr, "Warning: skipping non-alphanumeric token '%s'\n", buf);
            continue;
        }
        for (i = 0; i < reg_count; i++) {
            if (strcmp(registry[i].token, buf) == 0) {
                registry[i].frequency++;
                break;
            }
        }
        /* If the loop exhausted without a match, i == reg_count: new token. */
        if (i == reg_count) {
            if (reg_count == reg_cap) {
                reg_cap = reg_cap ? reg_cap * 2 : 16;
                registry = realloc(registry, reg_cap * sizeof(StringEntry));
                if (!registry) {
                    fprintf(stderr, "Error: out of memory\n");
                    exit(1);
                }
            }
            registry[reg_count].token     = strdup(buf);
            registry[reg_count].frequency = 1;
            reg_count++;
        }
    }

    /* Pass 2: build BST from registry in first-appearance order */
    for (i = 0; i < reg_count; i++) {
        Node *existing = bst_find(root, registry[i].frequency);
        if (existing) {
            node_add_string(existing, registry[i].token);
        } else {
            Node *new_node = make_node(registry[i].frequency, registry[i].token);
            root = bst_insert(root, new_node);
        }
        free(registry[i].token);
    }
    free(registry);

    return root;
}

void printPreorder(Node *root, int depth, FILE *out)
{
    if (!root) return;
    print_node(root, depth, out);
    printPreorder(root->left,  depth + 1, out);
    printPreorder(root->right, depth + 1, out);
}

void printInorder(Node *root, int depth, FILE *out)
{
    if (!root) return;
    printInorder(root->left,  depth + 1, out);
    print_node(root, depth, out);
    printInorder(root->right, depth + 1, out);
}

void printPostorder(Node *root, int depth, FILE *out)
{
    if (!root) return;
    printPostorder(root->left,  depth + 1, out);
    printPostorder(root->right, depth + 1, out);
    print_node(root, depth, out);
}

void free_tree(Node *root)
{
    int i;
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    for (i = 0; i < root->str_count; i++)
        free(root->strings[i]);
    free(root->strings);
    free(root);
}
