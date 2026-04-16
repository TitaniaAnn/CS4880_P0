#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "node.h"

static void exitError(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

/* Open an output file; abort with an error message if it fails. */
static FILE *open_output(const char *basename, const char *ext)
{
    char path[512];
    FILE *fp;
    snprintf(path, sizeof(path), "%s.%s", basename, ext);
    fp = fopen(path, "w");
    if (!fp) {
        char msg[560];
        snprintf(msg, sizeof(msg), "Error: cannot open output file '%s'", path);
        exitError(msg);
    }
    return fp;
}

int main(int argc, char *argv[])
{
    FILE       *in       = NULL;
    const char *basename = NULL;
    Node       *root     = NULL;
    FILE       *pre_fp, *in_fp, *post_fp;

    /* --- process command line arguments --- */
    if (argc > 2)
        exitError("Usage: P0 [basename]");

    if (argc == 1) {
        in       = stdin;
        basename = "out";
    } else {
        char infile[512];
        snprintf(infile, sizeof(infile), "%s.fs25s1", argv[1]);
        in = fopen(infile, "r");
        if (!in) {
            char msg[560];
            snprintf(msg, sizeof(msg), "Error: cannot open input file '%s'", infile);
            exitError(msg);
        }
        basename = argv[1];
    }

    /* --- build the tree (two passes handled inside buildTree) --- */
    root = buildTree(in);

    if (in != stdin)
        fclose(in);

    /* --- traverse the tree, generating output files --- */
    pre_fp  = open_output(basename, "preorder");
    in_fp   = open_output(basename, "inorder");
    post_fp = open_output(basename, "postorder");

    printPreorder (root, 0, pre_fp);
    printInorder  (root, 0, in_fp);
    printPostorder(root, 0, post_fp);

    fclose(pre_fp);
    fclose(in_fp);
    fclose(post_fp);

    free_tree(root);

    return 0;
}
