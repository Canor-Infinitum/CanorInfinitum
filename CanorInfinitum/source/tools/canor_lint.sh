#!/bin/sh
# canor_lint: AGENTS.md invariants 1-2 for the library (source/include, source/src):
# no floating point, no <math.h>, no heap.
#
# Comments and string literals are blanked first, keeping line numbers, so prose
# that says "a double would ..." never matches, while code after a comment, or a
# code line that starts with '*' (a dereference), still does.
#
# Run from the repository root. Exit 0 = clean, 1 = violations, 2 = no files.
set -eu

# rg prints '\' separators on Windows; normalize so sed doesn't eat them.
files=$(rg --files source/include source/src | tr '\\' '/')
if [ -z "$files" ]; then
  echo "canor_lint: no library files found (run from the repository root)" >&2
  exit 2
fi

pattern='\b(malloc|calloc|realloc|free)\s*\(|\b(float|double|sqrt)\b|math\.h'
hits=""
for f in $files; do
  found=$(awk '
    {
      out = ""; i = 1; n = length($0)
      while (i <= n) {
        c = substr($0, i, 1); d = substr($0, i, 2)
        if (inc) { if (d == "*/") { inc = 0; out = out "  "; i += 2 } else { out = out " "; i++ } continue }
        if (ins) { if (c == "\\") { out = out "  "; i += 2; continue } if (c == q) ins = 0; out = out " "; i++; continue }
        if (d == "/*") { inc = 1; out = out "  "; i += 2; continue }
        if (d == "//") break
        if (c == "\"" || c == "'\''") { ins = 1; q = c; out = out " "; i++; continue }
        out = out c; i++
      }
      ins = 0
      print out
    }' "$f" | rg -n "$pattern" || true)
  if [ -n "$found" ]; then
    hits="$hits$(printf '%s\n' "$found" | sed "s|^|$f:|")
"
  fi
done

if [ -n "$hits" ]; then
  printf '%s' "$hits"
  exit 1
fi
echo "canor_lint: clean"
