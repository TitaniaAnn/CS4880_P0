#!/usr/bin/env bash
# Integration tests for P0.
# Run from the project root; requires ./P0 to be built first.

PASS=0
FAIL=0
BINARY=./P0

if [ ! -x "$BINARY" ]; then
    echo "ERROR: $BINARY not found. Run 'make' first." >&2
    exit 1
fi

# Compare <prefix>.<traversal> to expected string; print diff on mismatch.
# Strips \r so tests pass on Windows (text-mode CRLF) and Linux alike.
_check() {
    local file="${1}.${2}" expected="$3" actual
    actual=$(tr -d '\r' < "$file" 2>/dev/null)
    if [ "$actual" = "$expected" ]; then return 0; fi
    echo "  ${2} mismatch:"
    diff <(printf '%s\n' "$expected") <(printf '%s\n' "$actual") | sed 's/^/    /'
    return 1
}

# Feed input via stdin, compare all three traversal files (basename "out").
run_test() {
    local name="$1" input="$2" exp_pre="$3" exp_in="$4" exp_post="$5"
    printf '%s' "$input" | "$BINARY" 2>/dev/null
    local ok=1
    _check out preorder  "$exp_pre"  || ok=0
    _check out inorder   "$exp_in"   || ok=0
    _check out postorder "$exp_post" || ok=0
    rm -f out.preorder out.inorder out.postorder
    if [ "$ok" -eq 1 ]; then
        printf 'PASS  %s\n' "$name"; PASS=$((PASS + 1))
    else
        printf 'FAIL  %s\n' "$name"; FAIL=$((FAIL + 1))
    fi
}

# ── stdin tests ───────────────────────────────────────────────────────────

run_test "single token" \
    "apple" \
    "1: apple" "1: apple" "1: apple"

run_test "all-unique tokens share one node" \
    "apple banana cherry" \
    "1: apple banana cherry" \
    "1: apple banana cherry" \
    "1: apple banana cherry"

# Tree: 3:apple → left:2:banana → left:1:cherry
run_test "left-skewed tree" \
    "apple apple apple banana banana cherry" \
    "$(printf '3: apple\n  2: banana\n    1: cherry')" \
    "$(printf '    1: cherry\n  2: banana\n3: apple')" \
    "$(printf '    1: cherry\n  2: banana\n3: apple')"

# Tree: 1:a → right:2:b → right:3:c
run_test "right-skewed tree" \
    "a b b c c c" \
    "$(printf '1: a\n  2: b\n    3: c')" \
    "$(printf '1: a\n  2: b\n    3: c')" \
    "$(printf '    3: c\n  2: b\n1: a')"

# apple and banana both freq=2, share one node; cherry freq=1, left child
run_test "equal-frequency tokens share one node" \
    "apple apple banana banana cherry" \
    "$(printf '2: apple banana\n  1: cherry')" \
    "$(printf '  1: cherry\n2: apple banana')" \
    "$(printf '  1: cherry\n2: apple banana')"

# Tree: 3:a, left=1:b{right=2:c}, right=4:d
run_test "root with both left and right children" \
    "a a a b c c d d d d" \
    "$(printf '3: a\n  1: b\n    2: c\n  4: d')" \
    "$(printf '  1: b\n    2: c\n3: a\n  4: d')" \
    "$(printf '    2: c\n  1: b\n  4: d\n3: a')"

# ── special cases ─────────────────────────────────────────────────────────

# Non-alphanumeric token must be absent from output and warned on stderr.
name="non-alphanumeric token skipped"
printf 'hello wor!ld goodbye' | "$BINARY" 2>_stderr.tmp
actual_pre=$(tr -d '\r' < out.preorder 2>/dev/null)
stderr_msg=$(cat _stderr.tmp)
rm -f _stderr.tmp out.preorder out.inorder out.postorder
ok=1
[ "$actual_pre" = "1: hello goodbye" ]        || { echo "  preorder: got '$actual_pre'"; ok=0; }
printf '%s' "$stderr_msg" | grep -q "wor!ld"  || { echo "  no warning on stderr"; ok=0; }
if [ "$ok" -eq 1 ]; then
    printf 'PASS  %s\n' "$name"; PASS=$((PASS + 1))
else
    printf 'FAIL  %s\n' "$name"; FAIL=$((FAIL + 1))
fi

# Empty input: output files must exist but be empty.
name="empty input"
printf '' | "$BINARY" 2>/dev/null
ok=1
for t in preorder inorder postorder; do
    [ -f "out.$t" ]   || { echo "  out.$t missing"; ok=0; }
    [ ! -s "out.$t" ] || { echo "  out.$t not empty"; ok=0; }
done
rm -f out.preorder out.inorder out.postorder
if [ "$ok" -eq 1 ]; then
    printf 'PASS  %s\n' "$name"; PASS=$((PASS + 1))
else
    printf 'FAIL  %s\n' "$name"; FAIL=$((FAIL + 1))
fi

# Named file argument: output files must use the given basename, not "out".
name="named file argument"
printf 'foo foo bar\n' > _test.fs25s1
"$BINARY" _test 2>/dev/null
ok=1
_check _test preorder  "$(printf '2: foo\n  1: bar')" || ok=0
_check _test inorder   "$(printf '  1: bar\n2: foo')" || ok=0
_check _test postorder "$(printf '  1: bar\n2: foo')" || ok=0
rm -f _test.fs25s1 _test.preorder _test.inorder _test.postorder
if [ "$ok" -eq 1 ]; then
    printf 'PASS  %s\n' "$name"; PASS=$((PASS + 1))
else
    printf 'FAIL  %s\n' "$name"; FAIL=$((FAIL + 1))
fi

# ── summary ───────────────────────────────────────────────────────────────
printf '\n%d passed, %d failed\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
