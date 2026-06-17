#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "node.h"

/* Print an error message to stderr and abort with a failure status.
   Used for every fatal condition (bad arguments, unopenable files). */
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
    /* Build "<basename>.<ext>", e.g. "out.preorder" or "P0_test3.inorder". */
    snprintf(path, sizeof(path), "%s.%s", basename, ext);
    fp = fopen(path, "w");
    if (!fp) {
        char msg[560];   /* a little larger than path[] to hold the prefix text too */
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
    /* Only "P0" or "P0 <name>" are legal; anything more is a usage error. */
    if (argc > 2)
        exitError("Usage: P0 [basename]");

    if (argc == 1) {
        /* No argument: read from stdin (keyboard, or a shell redirect such as
           "P0 < file"). The program can't tell these apart, so the output
           basename is the fixed string "out". */
        in       = stdin;
        basename = "out";
    } else {
        /* Argument given: it is a base name. Append the implicit ".fs25s1"
           extension (the extension is never typed on the command line). */
        char infile[512];
        snprintf(infile, sizeof(infile), "%s.fs25s1", argv[1]);
        in = fopen(infile, "r");
        if (!in) {
            /* Unreadable / missing input file is a fatal error. */
            char msg[560];
            snprintf(msg, sizeof(msg), "Error: cannot open input file '%s'", infile);
            exitError(msg);
        }
        basename = argv[1];   /* output files reuse the input's base name */
    }

    /* --- build the tree (two passes handled inside buildTree) --- */
    root = buildTree(in);

    /* Close the input file we opened, but never close stdin. */
    if (in != stdin)
        fclose(in);

    /* --- traverse the tree, generating output files --- */
    /* One file per traversal; each is opened (or we abort) before writing. */
    pre_fp  = open_output(basename, "preorder");
    in_fp   = open_output(basename, "inorder");
    post_fp = open_output(basename, "postorder");

    /* depth starts at 0 for the root, controlling output indentation. */
    printPreorder (root, 0, pre_fp);
    printInorder  (root, 0, in_fp);
    printPostorder(root, 0, post_fp);

    fclose(pre_fp);
    fclose(in_fp);
    fclose(post_fp);

    free_tree(root);   /* release every node and string before exiting */

    return 0;
}
