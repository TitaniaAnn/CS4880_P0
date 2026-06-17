# P0 Code Walkthrough

A guided tour of the P0 source so you can explain every line in a demo. The
program reads whitespace-delimited tokens, builds a binary search tree (BST)
keyed by each token's **frequency**, and writes three traversals to files.

---

## 1. The big picture

```
input tokens ──► Pass 1: count frequencies (registry)
                          │
                          ▼
                 Pass 2: build BST keyed by frequency
                          │
                          ▼
            preorder / inorder / postorder ──► three output files
```

The work is split exactly as the assignment's top-down decomposition asks:

| Task | Where it lives |
| --- | --- |
| 1. Process arguments, pick input source, set basename | `P0.c` (`main` + static helpers) |
| 2. Build the tree | `tree.c` → `buildTree` |
| 3. Traverse and write output | `tree.c` → `printPreorder/Inorder/Postorder` |
| Node type definition | `node.h` |

This matches the **Structural Requirements**: `main` and its argument/error
helpers in `P0.c`; `buildTree` and the three `print*` functions in `tree.c`
(declared in `tree.h`); the `Node` struct in its own `node.h`.

---

## 2. The data model — `node.h`

```c
typedef struct Node {
    int          frequency; /* BST key */
    char       **strings;   /* tokens sharing this frequency, first-appearance order */
    int          str_count; /* how many strings are stored */
    int          str_cap;   /* allocated capacity of strings[] */
    struct Node *left;
    struct Node *right;
} Node;
```

Key idea from the spec: **the BST key is the frequency, not the token.** Two
different tokens that appear the same number of times are "equal" and land in
the *same* node, so each node holds a *list* of strings (`strings`). The list
grows dynamically (`str_cap` doubles as needed) because we don't know in
advance how many tokens share a frequency.

---

## 3. `main` — argument handling and orchestration (`P0.c`)

### Choosing the input source

```c
if (argc > 2)
    exitError("Usage: P0 [basename]");      // too many args → abort, exit(1)

if (argc == 1) {
    in = stdin;  basename = "out";          // case 1/2: keyboard OR `P0 < file`
} else {
    snprintf(infile, ..., "%s.fs25s1", argv[1]);  // case 3: append extension
    in = fopen(infile, "r");
    if (!in) exitError(...);                // unreadable file → abort, exit(1)
    basename = argv[1];
}
```

This implements the three invocation cases precisely:

- **`P0`** and **`P0 < file`** are indistinguishable to the program — both read
  from `stdin`, and the basename is the constant `"out"`. The program never
  knows whether data came from the keyboard or a redirected file.
- **`P0 name`** appends the implicit `.fs25s1` extension itself (the extension
  is *not* allowed on the command line) and uses `name` as the basename.

Both error paths (`argc > 2`, unopenable file) print a message and `exit(1)`,
satisfying the "abort with an appropriate message" requirement.

### The three steps

```c
root = buildTree(in);                 // step 2
if (in != stdin) fclose(in);          // only close files we opened

pre_fp  = open_output(basename, "preorder");   // step 3
in_fp   = open_output(basename, "inorder");
post_fp = open_output(basename, "postorder");
printPreorder (root, 0, pre_fp);
printInorder  (root, 0, in_fp);
printPostorder(root, 0, post_fp);
... fclose all three ...
free_tree(root);                      // tidy cleanup, no leaks
```

`open_output` is a small static helper that builds `basename.ext`, opens it for
writing, and aborts if it can't — keeping `main` readable. Note `stdin` is
deliberately *not* closed.

---

## 4. `buildTree` — the two-pass algorithm (`tree.c`)

The spec calls for **two passes** because a token's frequency is unknown until
the whole input has been seen.

### Pass 1 — count frequencies into a registry

```c
while (fscanf(in, "%1023s", buf) == 1) {
    if (!is_alphanumeric(buf)) {                 // +10% validation
        fprintf(stderr, "Warning: skipping ... '%s'\n", buf);
        continue;
    }
    // linear search registry for buf; if found, frequency++
    // else append a new {strdup(buf), frequency=1} entry (array doubles as needed)
}
```

- `%1023s` reads one whitespace-delimited token at a time, bounded so `buf`
  (1024 bytes) can't overflow.
- **Validation** (`is_alphanumeric`): the spec says data is letters and digits
  only; any token with another character triggers a stderr warning and is
  skipped. This earns the `+10%` validation bonus. `isalnum` is called with the
  char cast to `unsigned char` to avoid undefined behavior on negative values.
- The registry preserves **first-appearance order**, which later determines the
  left-to-right order of strings inside a shared node.

### Pass 2 — build the BST from the registry

```c
for (i = 0; i < reg_count; i++) {
    Node *existing = bst_find(root, registry[i].frequency);
    if (existing)
        node_add_string(existing, registry[i].token);   // same freq → same node
    else
        root = bst_insert(root, make_node(freq, token)); // new freq → new node
    free(registry[i].token);
}
```

For each registered token, in first-appearance order:

- If a node with that **frequency** already exists, append the token to its
  string list (this is how equal-frequency tokens share a node, listed in
  first-appearance order).
- Otherwise create a new node and insert it by frequency.

`bst_insert` is a textbook BST insert: smaller frequency goes left, larger goes
right. Because equal frequencies are caught by `bst_find` *before* insertion,
`bst_insert` never has to handle a duplicate key — which is exactly why its
`else` branch can safely send `>=` to the right without creating duplicates.

> **Worked example** (the official `P0_test3` input). Frequencies in first-
> appearance order: `adam 5, 123 2, 35 5, susan 3, daniel 1, 12 1, 23 1, x 7,
> y 6`. Building by frequency yields:
>
> ```
>            5: adam 35
>           /          \
>      2: 123          7: x
>      /     \         /
> 1: daniel  3: susan 6: y
>    12 23
> ```
>
> `adam` and `35` share node `5`; `daniel`, `12`, `23` share node `1` — each in
> first-appearance order. This reproduces the instructor's example exactly.

---

## 5. Traversals and printing (`tree.c`)

All three traversals share one formatter:

```c
static void print_node(Node *n, int depth, FILE *out) {
    for (i = 0; i < 2 * depth; i++) fputc(' ', out);   // indent = 2 × depth
    fprintf(out, "%d:", n->frequency);                 // "<freq>:"
    for (i = 0; i < n->str_count; i++)
        fprintf(out, " %s", n->strings[i]);            // " <token>" each
    fputc('\n', out);
}
```

This is the exact output format from the spec: `2*depth` leading spaces (root at
depth 0), the frequency, a colon, then each string preceded by a space.

The three traversals differ only in *when* the root is printed relative to its
children — straight from the 3130 textbook definitions:

```c
printPreorder : node, left, right      // root first
printInorder  : left, node, right      // root in the middle
printPostorder: left, right, node      // root last
```

Each recurses with `depth + 1` so indentation tracks tree depth. An empty tree
(`root == NULL`) prints nothing, which is why empty input correctly yields three
empty files.

---

## 6. Cleanup — `free_tree`

A postorder recursive free: free both subtrees, then each string in the node,
then the node's `strings` array, then the node itself. Valgrind confirms
**0 bytes leaked / 0 errors** on the test3 run.

---

## 7. How it's verified

- `make` builds cleanly with `-Wall -Wextra -pedantic -std=c11` (no warnings).
- `make test` runs `run_tests.sh`: 9 integration tests covering single tokens,
  shared nodes, skewed trees, both-children trees, validation, empty input, and
  named-file output — all pass.
- The official `P0_test3` example reproduces the instructor's preorder, inorder,
  and postorder output byte-for-byte.

---

## 8. Things to be ready to discuss in a demo

- **Why two passes?** Frequency (the BST key) isn't known until all input is
  read, so Pass 1 counts and Pass 2 builds.
- **Why a list of strings per node?** Equal-frequency tokens are "equal" keys
  and share a node by the spec.
- **Why first-appearance order matters twice:** it sets both the order strings
  are listed within a node *and* the order nodes are created (the tree is never
  rebalanced, so creation order shapes the tree).
- **Possible refinement:** Pass 1 looks up tokens with a linear scan of the
  registry — `O(tokens × unique tokens)`. Fine for assignment-sized inputs; a
  hash table would make it `O(tokens)` if scaling up ever mattered.
