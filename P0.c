#define _POSIX_C_SOURCE 200809L  /* must precede all includes: exposes strdup under -std=c11 -pedantic */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "node.h"

/* Print an error message to stderr and abort with a failure status.
   Used for every "give up" path (bad arguments, unopenable files). */
static void exitError(const char *msg)
{
    fprintf(stderr, "%s\n", msg);   /* diagnostics go to stderr, never to the output files */
    exit(1);                        /* non-zero status signals failure to the shell */
}

/* Open "<basename>.<ext>" for writing; abort with an error message if it fails. */
static FILE *open_output(const char *basename, const char *ext)
{
    char path[512];
    FILE *fp;
    snprintf(path, sizeof(path), "%s.%s", basename, ext);  /* compose filename safely (no overflow) */
    fp = fopen(path, "w");
    if (!fp) {                                             /* e.g. permission denied / bad path */
        char msg[560];
        snprintf(msg, sizeof(msg), "Error: cannot open output file '%s'", path);
        exitError(msg);
    }
    return fp;
}

int main(int argc, char *argv[])
{
    FILE       *in       = NULL;   /* where tokens are read from (a file or stdin) */
    const char *basename = NULL;   /* prefix for the three output filenames */
    Node       *root     = NULL;   /* root of the frequency-ordered BST */
    FILE       *pre_fp, *in_fp, *post_fp;

    /* --- Task 1: process command line arguments --- */
    if (argc > 2)                                  /* program takes at most one argument */
        exitError("Usage: P0 [basename]");

    if (argc == 1) {
        /* No argument: read from standard input. This covers both bare keyboard
           input ("P0") and shell redirection ("P0 < file") — the program cannot
           tell them apart, and per the spec it must not try. Output base is "out". */
        in       = stdin;
        basename = "out";
    } else {
        /* One argument: it is a basename. Append the implicit ".fs25s1" extension
           ourselves (the extension is NOT allowed on the command line) and open it. */
        char infile[512];
        snprintf(infile, sizeof(infile), "%s.fs25s1", argv[1]);
        in = fopen(infile, "r");
        if (!in) {                                 /* unreadable / missing file -> abort */
            char msg[560];
            snprintf(msg, sizeof(msg), "Error: cannot open input file '%s'", infile);
            exitError(msg);
        }
        basename = argv[1];                        /* outputs use the given name */
    }

    /* --- Task 2: build the tree (both passes happen inside buildTree) --- */
    root = buildTree(in);

    if (in != stdin)                               /* close real files; leave stdin alone */
        fclose(in);

    /* --- Task 3: traverse the tree three ways, each into its own file --- */
    pre_fp  = open_output(basename, "preorder");
    in_fp   = open_output(basename, "inorder");
    post_fp = open_output(basename, "postorder");

    printPreorder (root, 0, pre_fp);               /* depth starts at 0 for the root */
    printInorder  (root, 0, in_fp);
    printPostorder(root, 0, post_fp);

    fclose(pre_fp);
    fclose(in_fp);
    fclose(post_fp);

    free_tree(root);                               /* release all heap memory */

    return 0;                                      /* success */
}
