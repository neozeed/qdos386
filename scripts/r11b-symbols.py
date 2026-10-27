#!/usr/bin/env python3
"""List the functions exported by libc.a and libqdos.a, from the C sources.
Prints a Markdown table (symbol, library, source file) on stdout.  Only
non-static function definitions and the documented objects are listed.
Usage: python3 scripts/r11b-symbols.py  (run from the repository root)."""
import glob, re

DEF = re.compile(r'^(?:[A-Za-z_][\w \t\*]*?)\s\**([A-Za-z_]\w*)\s*\(', re.M)
SKIP = {"if", "while", "for", "switch", "return", "sizeof"}

def exported(path):
    text = open(path).read()
    # drop comments and preprocessor lines, keep definitions at column 0
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    text = re.sub(r'^#.*$', '', text, flags=re.M)
    out = []
    for m in DEF.finditer(text):
        name = m.group(1)
        line_start = text.rfind('\n', 0, m.start()) + 1
        head = text[line_start:m.end()]
        if head.lstrip().startswith("static") or name in SKIP:
            continue
        if re.search(r';\s*$', text[m.end():m.end() + 200].split('\n')[0]):
            continue  # a prototype, not a definition
        out.append(name)
    return out

rows = []
for src in sorted(glob.glob("sdk/libc/*.c")):
    for name in exported(src):
        rows.append((name, "libc.a", src))
for src in ["sdk/lib/qdos.c"]:
    for name in exported(src):
        rows.append((name, "libqdos.a", src))
print("| Symbol | Library | Source |")
print("|---|---|---|")
for name, lib, src in rows:
    print("| `%s` | %s | %s |" % (name, lib, src))
print()
print("Total: %d exported functions (%d in libc.a, %d in libqdos.a)." % (
    len(rows), sum(1 for r in rows if r[1] == "libc.a"), sum(1 for r in rows if r[1] == "libqdos.a")))
