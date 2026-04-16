#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tree.h"
#include "node.h"

/* Returns 1 if every character in s is a letter or digit, 0 otherwise. */
static int is_alphanumeric(const char *s)
{
    for (; *s; s++)
        if (!isalnum((unsigned char)*s))
            return 0;
    return 1;
}

/* Grow the registry by doubling when full. */
static StringEntry *registry_append(StringEntry *reg, int *count, int *cap,
                                    const char *token)
{
    if (*count == *cap) {
        *cap = (*cap == 0) ? 16 : (*cap * 2);
        reg = realloc(reg, *cap * sizeof(StringEntry));
        if (!reg) {
            fprintf(stderr, "Error: out of memory\n");
            exit(1);
        }
    }
    reg[*count].token     = strdup(token);
    reg[*count].frequency = 1;
    reg[*count].first_idx = *count;
    (*count)++;
    return reg;
}

/* Open an output file; abort with an error message if it fails. */
static FILE *open_output(const char *basename, const char *ext)
{
    char path[512];
    FILE *fp;
    snprintf(path, sizeof(path), "%s.%s", basename, ext);
    fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "Error: cannot open output file '%s'\n", path);
        exit(1);
    }
    return fp;
}

int main(int argc, char *argv[])
{
    FILE        *in       = NULL;
    const char  *basename = NULL;
    StringEntry *registry = NULL;
    int          reg_count = 0, reg_cap = 0;
    char         buf[1024];
    Node        *root = NULL;
    FILE        *pre_fp, *in_fp, *post_fp;
    int          i;

    /* --- argument handling --- */
    if (argc == 1) {
        in       = stdin;
        basename = "out";
    } else if (argc == 2) {
        char infile[512];
        snprintf(infile, sizeof(infile), "%s.fs25s1", argv[1]);
        in = fopen(infile, "r");
        if (!in) {
            fprintf(stderr, "Error: cannot open input file '%s'\n", infile);
            return 1;
        }
        basename = argv[1];
    } else {
        fprintf(stderr, "Usage: P0 [basename]\n");
        return 1;
    }

    /* --- pass 1: read tokens, build registry --- */
    while (fscanf(in, "%1023s", buf) == 1) {
        if (!is_alphanumeric(buf)) {
            fprintf(stderr, "Warning: skipping non-alphanumeric token '%s'\n", buf);
            continue;
        }
        /* Search registry for existing entry */
        for (i = 0; i < reg_count; i++) {
            if (strcmp(registry[i].token, buf) == 0) {
                registry[i].frequency++;
                break;
            }
        }
        if (i == reg_count)
            registry = registry_append(registry, &reg_count, &reg_cap, buf);
    }

    if (in != stdin)
        fclose(in);

    /* --- pass 2: build BST --- */
    root = buildTree(registry, reg_count);

    /* --- open output files and write traversals --- */
    pre_fp  = open_output(basename, "preorder");
    in_fp   = open_output(basename, "inorder");
    post_fp = open_output(basename, "postorder");

    printPreorder (root, 0, pre_fp);
    printInorder  (root, 0, in_fp);
    printPostorder(root, 0, post_fp);

    fclose(pre_fp);
    fclose(in_fp);
    fclose(post_fp);

    /* --- cleanup --- */
    free_tree(root);
    for (i = 0; i < reg_count; i++)
        free(registry[i].token);
    free(registry);

    return 0;
}
