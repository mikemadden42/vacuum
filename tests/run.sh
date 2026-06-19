#!/usr/bin/env bash
#
# Functional tests for vacuum. Builds nothing itself; point it at a binary via
# $VACUUM or the first argument, otherwise it defaults to ../build/vacuum.
#
# Usage: tests/run.sh [path-to-vacuum]

set -u

script_dir=$(cd "$(dirname "$0")" && pwd)
VACUUM=${1:-${VACUUM:-"$script_dir/../build/vacuum"}}

if [[ ! -x "$VACUUM" ]]; then
    echo "error: vacuum binary not found at '$VACUUM'" >&2
    echo "build it first (cmake --build build) or pass a path" >&2
    exit 2
fi

# Resolve to an absolute path: the tests cd into temp dirs, so a relative
# binary path would no longer be found.
VACUUM="$(cd "$(dirname "$VACUUM")" && pwd)/$(basename "$VACUUM")"

pass=0
fail=0

ok() { printf 'ok   - %s\n' "$1"; pass=$((pass + 1)); }
no() { printf 'FAIL - %s\n' "$1"; fail=$((fail + 1)); }

# assert that a file exists
assert_file() {
    if [[ -f "$1" ]]; then ok "$2"; else no "$2 (missing: $1)"; fi
}

# assert that a path does NOT exist
assert_absent() {
    if [[ ! -e "$1" ]]; then ok "$2"; else no "$2 (present: $1)"; fi
}

# assert that a path is a symlink (regardless of whether its target exists)
assert_symlink() {
    if [[ -L "$1" ]]; then ok "$2"; else no "$2 (not a symlink: $1)"; fi
}

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# --- core: bucketing, dotfile / extensionless / symlink skipping ---
src="$work/core"
mkdir -p "$src"
( cd "$src"
  touch report.pdf notes.md a.txt b.txt archive.tar.gz noext .hidden.txt
  ln -s a.txt link.txt
  "$VACUUM" . Documents >/dev/null
)
assert_file   "$src/Documents/pdf/report.pdf"   "pdf bucketed"
assert_file   "$src/Documents/md/notes.md"      "md bucketed"
assert_file   "$src/Documents/txt/a.txt"        "txt bucketed (a)"
assert_file   "$src/Documents/txt/b.txt"        "txt bucketed (b)"
assert_file   "$src/Documents/gz/archive.tar.gz" "multi-dot uses last ext"
assert_absent "$src/Documents/noext"            "extensionless skipped"
assert_file   "$src/noext"                       "extensionless left in place"
assert_file   "$src/.hidden.txt"                 "dotfile left in place"
assert_symlink "$src/link.txt"                   "symlink left in place"
assert_absent "$src/Documents/txt/link.txt"     "symlink not moved"

# --- already-exists path: file is left in source, message printed ---
src="$work/exists"
mkdir -p "$src/Documents/txt"
( cd "$src"
  touch a.txt
  echo keep > Documents/txt/a.txt
  out=$("$VACUUM" . Documents)
  echo "$out" | grep -q "already exists" || exit 3
)
if [[ $? -eq 0 ]]; then ok "already-exists prints message"; else no "already-exists prints message"; fi
assert_file "$src/a.txt"                  "already-exists leaves source file"
[[ "$(cat "$src/Documents/txt/a.txt")" == "keep" ]] \
    && ok "already-exists does not overwrite" \
    || no "already-exists does not overwrite"

# --- custom dest_base, with a trailing slash ---
# (// and / are the same path to the kernel, so verify via the printed message)
src="$work/dest"
mkdir -p "$src"
out=$( cd "$src"; touch x.log; "$VACUUM" . sorted/ )
assert_file "$src/sorted/log/x.log"  "custom dest_base honored"
if [[ "$out" != *"//"* ]]; then
    ok "trailing slash not duplicated in output"
else
    no "trailing slash not duplicated in output (got: $out)"
fi

# --- cross-filesystem move (EXDEV fallback) ---
# Needs a writable directory on a different filesystem; /dev/shm is the usual
# candidate on Linux. Skipped when no second filesystem is available.
skip() { printf 'skip - %s\n' "$1"; }
xfs_dev() { stat -c '%d' "$1" 2>/dev/null; }
otherfs=""
for cand in /dev/shm /run/user/"$(id -u)"; do
    if [[ -d "$cand" && -w "$cand" && "$(xfs_dev "$cand")" != "$(xfs_dev "$work")" ]]; then
        otherfs="$cand"; break
    fi
done
if [[ -n "$otherfs" ]]; then
    src="$work/xdev"
    dst=$(mktemp -d "$otherfs/vac.XXXXXX")
    mkdir -p "$src"
    ( cd "$src"; printf 'payload\n' > move.bin; chmod 640 move.bin
      "$VACUUM" . "$dst" >/dev/null )
    assert_file "$dst/bin/move.bin"          "EXDEV: file copied to other fs"
    assert_absent "$src/move.bin"            "EXDEV: source removed"
    [[ "$(cat "$dst/bin/move.bin" 2>/dev/null)" == "payload" ]] \
        && ok "EXDEV: content intact" || no "EXDEV: content intact"
    [[ "$(stat -c '%a' "$dst/bin/move.bin" 2>/dev/null)" == "640" ]] \
        && ok "EXDEV: permissions preserved" || no "EXDEV: permissions preserved"
    rm -rf "$dst"
else
    skip "EXDEV cross-filesystem move (no second filesystem available)"
fi

# --- CLI behavior ---
"$VACUUM" --help >/dev/null 2>&1 && ok "--help exits 0" || no "--help exits 0"
"$VACUUM" -h     >/dev/null 2>&1 && ok "-h exits 0"     || no "-h exits 0"
"$VACUUM" --bogus >/dev/null 2>&1 && no "unknown option fails" || ok "unknown option fails"
"$VACUUM" a b c   >/dev/null 2>&1 && no "too many args fails"  || ok "too many args fails"
"$VACUUM" /no/such/dir >/dev/null 2>&1 && no "missing dir fails" || ok "missing dir fails"

echo
echo "passed: $pass, failed: $fail"
[[ $fail -eq 0 ]]
