# CS 4880 P0 — Code Walkthrough

A developer-oriented tour of the codebase: control flow, data structures, and a
full execution trace. For the requirement-by-requirement compliance mapping, see
[P0_walkthrough.html](P0_walkthrough.html). This document explains **how the code
works**; that one explains **why each decision satisfies the spec**.

---

## 1. What the program does

Reads whitespace-delimited word tokens from a file or stdin, counts how often each
distinct word appears, and builds a **binary search tree keyed by that frequency**
(not by the word itself). It then writes the tree to three files using preorder,
inorder, and postorder traversals.

Words that appear the same number of times collapse into a **single node** holding
a list of those words, kept in first-appearance order.

```
input:  apple banana apple cherry banana apple
counts: apple=3, banana=2, cherry=1
tree:        3:apple
            /
          2:banana
          /
        1:cherry
```

---

## 2. Build & run

```bash
make            # builds ./P0 using the required gcc flags
make clean      # removes objects, binary, and stray output files
make test       # builds, then runs run_tests.sh

./P0                       # read keyboard tokens, Ctrl+D to end  -> out.*
./P0 < data.fs25s1         # stdin redirect (full filename)       -> out.*
./P0 data                  # reads data.fs25s1                    -> data.*
```

The required compile flags are `-Wall -Wextra -pedantic -std=c11 -g`
([Makefile:2](Makefile#L2)). `_POSIX_C_SOURCE 200809L` is defined at the top of
both `.c` files so `strdup` is visible under `-std=c11 -pedantic`
([P0.c:1](P0.c#L1), [tree.c:1](tree.c#L1)).

---

## 3. Source layout

| File | Role |
|------|------|
| [node.h](node.h) | `Node` struct — the BST node type, isolated in its own header |
| [tree.h](tree.h) | Public API: `buildTree`, the three `print*` functions, `free_tree` |
| [tree.c](tree.c) | Tree construction, traversal, and all internal helpers |
| [P0.c](P0.c) | `main` plus the argument/error/output helpers that support it |
| [Makefile](Makefile) | Build rules |
| [run_tests.sh](run_tests.sh) | Integration tests comparing output to expected strings |

The split is deliberate: argument handling and error reporting live with `main`
in `P0.c`; all tree logic lives in `tree.c`; the node type sits alone in `node.h`.

---

## 4. Data structures

### 4.1 `Node` — the BST node ([node.h:4-11](node.h#L4-L11))

```c
typedef struct Node {
    int          frequency; /* BST key */
    char       **strings;   /* dynamic array of words, first-appearance order */
    int          str_count; /* number of words stored */
    int          str_cap;   /* allocated capacity of strings */
    struct Node *left;
    struct Node *right;
} Node;
```

`frequency` is the BST key. `strings` is a growable array because any number of
distinct words can share one frequency, and the spec requires keeping each word
exactly once in appearance order. `str_count` / `str_cap` track the live count vs.
allocated capacity so the array can double in place.

### 4.2 `StringEntry` — the pass-1 registry ([tree.c:12-15](tree.c#L12-L15))

```c
typedef struct {
    char *token;
    int   frequency;
} StringEntry;
```

This is an **internal** type (`static`-file scope, not in any header) used only as
the intermediate container between the two passes. One entry per distinct word,
holding the word and its running count.

---

## 5. `main()` control flow ([P0.c:30-78](P0.c#L30-L78))

`main` is the top-down decomposition into three tasks.

### Task 1 — argument processing ([P0.c:37-54](P0.c#L37-L54))

```c
if (argc > 2)
    exitError("Usage: P0 [basename]");

if (argc == 1) {              /* P0  or  P0 < file  */
    in       = stdin;
    basename = "out";
} else {                      /* P0 somename        */
    snprintf(infile, sizeof(infile), "%s.fs25s1", argv[1]);
    in = fopen(infile, "r");
    if (!in) { /* error + exit(1) */ }
    basename = argv[1];
}
```

Three outcomes:
- **More than one argument** → abort with a usage message.
- **No argument** → read from `stdin`; output basename is the literal `"out"`. This
  single branch covers both bare keyboard input and `P0 < file` — the program
  cannot distinguish them, and per spec it must not try.
- **One argument** → treat it as a basename, append `.fs25s1`, and open that file.
  The extension is added *by the program*; passing it on the command line is wrong
  by design and produces a `name.fs25s1.fs25s1` miss.

### Task 2 — build ([P0.c:57-60](P0.c#L57-L60))

```c
root = buildTree(in);
if (in != stdin)
    fclose(in);
```

All tree logic is delegated to `buildTree`. Note the guard: a real file handle is
closed, but `stdin` is left open.

### Task 3 — traverse and write ([P0.c:63-75](P0.c#L63-L75))

```c
pre_fp  = open_output(basename, "preorder");
in_fp   = open_output(basename, "inorder");
post_fp = open_output(basename, "postorder");

printPreorder (root, 0, pre_fp);
printInorder  (root, 0, in_fp);
printPostorder(root, 0, post_fp);

fclose(pre_fp); fclose(in_fp); fclose(post_fp);
free_tree(root);
```

`open_output(basename, ext)` builds `basename.ext` and opens it, aborting if the
open fails ([P0.c:16-28](P0.c#L16-L28)). Each traversal starts at depth `0` (the
root). Finally the whole tree is freed.

### Helpers in P0.c

- `exitError(msg)` ([P0.c:9-13](P0.c#L9-L13)) — prints to **stderr** and
  `exit(1)`. Used for every abort path, keeping stdout/output files clean.
- `open_output(basename, ext)` ([P0.c:16-28](P0.c#L16-L28)) — `snprintf` the
  filename, `fopen` for write, abort on failure.

---

## 6. `buildTree()` — the two-pass core ([tree.c:94-144](tree.c#L94-L144))

Frequencies aren't known until the entire input is read, so the tree can't be
built in one streaming pass without later restructuring. The solution: count
everything first (Pass 1), then build the tree from final counts (Pass 2).

### Pass 1 — accumulate counts into the registry ([tree.c:102-128](tree.c#L102-L128))

```c
while (fscanf(in, "%1023s", buf) == 1) {
    if (!is_alphanumeric(buf)) {
        fprintf(stderr, "Warning: skipping non-alphanumeric token '%s'\n", buf);
        continue;
    }
    for (i = 0; i < reg_count; i++)
        if (strcmp(registry[i].token, buf) == 0) {
            registry[i].frequency++;
            break;
        }
    if (i == reg_count) {           /* loop ran to the end: new token */
        /* grow registry if full, then append a new StringEntry */
        registry[reg_count].token     = strdup(buf);
        registry[reg_count].frequency = 1;
        reg_count++;
    }
}
```

Key mechanics:
- `fscanf(in, "%1023s", buf)` skips leading whitespace and reads one token of up to
  1023 chars (`buf` is 1024 with the null terminator). It returns `1` per token and
  `EOF` at end, which terminates the loop.
- **Validation (+10%)**: `is_alphanumeric` ([tree.c:17-24](tree.c#L17-L24)) rejects
  any token containing a non-`isalnum` character; rejected tokens warn and `continue`,
  never entering the registry. The `(unsigned char)` cast avoids undefined behavior
  passing a negative `char` to `isalnum`.
- The inner `for` is a **linear search** for an existing entry. The classic C idiom
  here is the post-loop test `if (i == reg_count)`: if the loop `break`s on a match,
  `i < reg_count`; if it runs to completion, `i == reg_count` signals "not found".
- **Unknown input size** is handled by a doubling dynamic array: capacity starts at
  16 and doubles on each `realloc` ([tree.c:116-123](tree.c#L116-L123)).

At the end of Pass 1 the registry holds one entry per distinct word, in
first-appearance order, each with its final frequency.

### Pass 2 — construct the BST ([tree.c:130-143](tree.c#L130-L143))

```c
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
```

Walking the registry in first-appearance order, for each word:
- `bst_find` ([tree.c:56-64](tree.c#L56-L64)) searches by frequency. **If a node
  with that frequency already exists**, the word is appended to that node's list
  (`node_add_string`) — this is how equal-frequency words share a node.
- **Otherwise** a fresh node is created (`make_node`) and inserted (`bst_insert`).

Because all frequencies are final before Pass 2 begins, a node's key never changes
after insertion, so the tree is **never restructured** — it only grows. Each
registry token is freed right after it's copied into the tree, and the registry
array itself is freed at the end.

---

## 7. Internal helpers in tree.c

### Registry/node growth — `node_add_string` ([tree.c:28-40](tree.c#L28-L40))

```c
if (n->str_count == n->str_cap) {
    n->str_cap = n->str_cap ? n->str_cap * 2 : 4;   /* 0 -> 4, then double */
    n->strings = realloc(n->strings, n->str_cap * sizeof(char *));
    /* abort on NULL */
}
n->strings[n->str_count++] = strdup(s);
```

Same doubling strategy as the registry, but starting at 4 slots. The word is
`strdup`'d so the node owns its own copy independent of the registry buffer.

### `make_node` ([tree.c:42-52](tree.c#L42-L52))

`calloc`s a zeroed node (so all counts/pointers start at 0/NULL), sets the
frequency, and seeds it with its first string via `node_add_string`.

### `bst_find` ([tree.c:56-64](tree.c#L56-L64)) — iterative search

```c
while (root) {
    if (frequency == root->frequency) return root;
    root = (frequency < root->frequency) ? root->left : root->right;
}
return NULL;
```

Standard BST descent: equal key → found; smaller → go left; larger → go right.

### `bst_insert` ([tree.c:68-77](tree.c#L68-L77)) — recursive insert

```c
if (!root) return new_node;
if (new_node->frequency < root->frequency)
    root->left  = bst_insert(root->left,  new_node);
else
    root->right = bst_insert(root->right, new_node);
return root;
```

Inserts strictly by frequency: **smaller goes left, larger goes right**. The
equal-frequency case never reaches `bst_insert` because Pass 2 routes duplicates
to `node_add_string` first ([tree.c:66-67](tree.c#L66-L67) explains this invariant).

### `print_node` ([tree.c:81-90](tree.c#L81-L90)) — the line format

```c
for (i = 0; i < 2 * depth; i++) fputc(' ', out);   /* indent = 2 * depth */
fprintf(out, "%d:", n->frequency);                  /* "<freq>:"          */
for (i = 0; i < n->str_count; i++)
    fprintf(out, " %s", n->strings[i]);             /* " word" each       */
fputc('\n', out);
```

Produces lines like `  2: banana cherry` — two spaces per depth level, the
frequency, a colon, then each word prefixed by a space.

---

## 8. The three traversals ([tree.c:146-168](tree.c#L146-L168))

All three share the same recursive shape; only the position of `print_node`
relative to the recursive calls differs.

```c
void printPreorder(Node *root, int depth, FILE *out) {
    if (!root) return;
    print_node(root, depth, out);                 /* root  */
    printPreorder(root->left,  depth + 1, out);    /* left  */
    printPreorder(root->right, depth + 1, out);    /* right */
}
```

| Function | Order | `print_node` position |
|----------|-------|-----------------------|
| `printPreorder`  ([tree.c:146](tree.c#L146)) | root, left, right | before both calls |
| `printInorder`   ([tree.c:154](tree.c#L154)) | left, root, right | between the calls |
| `printPostorder` ([tree.c:162](tree.c#L162)) | left, right, root | after both calls |

`depth` is **structural**, not visit-order: every recursive call passes
`depth + 1`, so both children of a depth-`d` node print at depth `d+1` regardless
of which traversal is running. A `NULL` child is the recursion base case and prints
nothing — which is also why empty input (a `NULL` root) yields three empty files.

Because the BST is keyed by frequency with larger-to-the-right, **inorder output is
always ascending frequency**.

---

## 9. Memory ownership

Every heap allocation has exactly one owner and one free site.

| Allocation | Allocated in | Freed in |
|------------|--------------|----------|
| `registry[i].token` | `strdup`, Pass 1 | end of Pass 2 loop ([tree.c:139](tree.c#L139)) |
| `registry` array | `realloc`, Pass 1 | after Pass 2 ([tree.c:141](tree.c#L141)) |
| `node->strings[i]` | `strdup` in `node_add_string` | `free_tree` ([tree.c:177](tree.c#L177)) |
| `node->strings` array | `realloc` in `node_add_string` | `free_tree` ([tree.c:178](tree.c#L178)) |
| each `Node` | `calloc` in `make_node` | `free_tree` ([tree.c:179](tree.c#L179)) |

`free_tree` ([tree.c:170-180](tree.c#L170-L180)) is a postorder walk: it frees both
subtrees, then the node's strings, then the node itself — children before parents,
so no pointer is used after free.

---

## 10. End-to-end trace

Input (the spec's `P0_test3`):

```
adam 123 adam 35 susan daniel adam 123 35 35 35 35 susan susan adam adam 12 23 x x x x x x x y y y y y y
```

**Pass 1** — registry in first-appearance order, with final counts:

| token | adam | 123 | 35 | susan | daniel | 12 | 23 | x | y |
|-------|------|-----|----|-------|--------|----|----|----|----|
| freq  | 5    | 2   | 5  | 3     | 1      | 1  | 1  | 7  | 6  |

**Pass 2** — insert in that order, routing equal frequencies into shared nodes:

1. `adam`(5) → root `5:[adam]`
2. `123`(2) → 2<5, left → `2:[123]`
3. `35`(5) → freq 5 found at root → append → `5:[adam 35]`
4. `susan`(3) → 3<5 left to 2, 3>2 right → `3:[susan]`
5. `daniel`(1) → 1<5,1<2 left → `1:[daniel]`
6. `12`(1) → freq 1 found → append → `1:[daniel 12]`
7. `23`(1) → append → `1:[daniel 12 23]`
8. `x`(7) → 7>5 right → `7:[x]`
9. `y`(6) → 6>5 right to 7, 6<7 left → `6:[y]`

Resulting tree:

```
              5: adam 35
             /          \
         2: 123          7: x
        /      \        /
1: daniel 12 23  3: susan  6: y
```

**Outputs** (verified to match the spec's reference exactly):

```
preorder              inorder                postorder
5: adam 35              1: daniel 12 23        1: daniel 12 23
  2: 123              2: 123                   3: susan
    1: daniel 12 23      3: susan             2: 123
    3: susan          5: adam 35                 6: y
  7: x                    6: y                 7: x
    6: y                7: x                 5: adam 35
```

(Indentation in the inorder/postorder columns above is schematic; the real files
use exactly `2 * depth` leading spaces per [print_node](tree.c#L81).)

---

## 11. Edge cases & error handling

| Situation | Behavior |
|-----------|----------|
| `argc > 2` | `Usage: P0 [basename]` → stderr, `exit(1)` |
| input file unreadable | `Error: cannot open input file '...'` → stderr, `exit(1)` |
| output file unopenable | `Error: cannot open output file '...'` → stderr, `exit(1)` |
| empty input | `NULL` root → three empty files |
| non-alphanumeric token | warning to stderr, token skipped (never affects the tree) |
| extension passed on cmdline | looks for `name.fs25s1.fs25s1` → open fails → abort |
| out-of-memory (`realloc`/`calloc`) | `Error: out of memory` → stderr, `exit(1)` |

All abort paths route through `stderr` + `exit(1)`, so the three traversal files
never contain diagnostic noise.
