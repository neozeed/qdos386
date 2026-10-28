#!/usr/bin/env python3
"""R11L: compare two GCC 1.40 i386 assembly files modulo the string-literal pool.
The pool differs between the historical cc1 built for a 32-bit host (identical string literals are merged, as in the
original) and the 64-bit-host repair build (not merged).  Canonical form: each ".LCn"/"LCn:" string definition
(label + .ascii/.byte lines until the next label or directive) is removed and every use of LCn is replaced by the string
text.  Two files are equivalent if their canonical forms are equal.  Exit 0: identical, 1: equivalent only modulo the pool,
2: different.
usage: r11l-sdiff.py A.S B.S"""
import re, sys


def canon(path):
    lines = open(path).read().split('\n')
    pool, out, i = {}, [], 0
    while i < len(lines):
        m = re.match(r'^(LC\d+):\s*$', lines[i])
        if m:
            j, body = i + 1, []
            while j < len(lines) and re.match(r'^\s*\.(ascii|byte|asciz)\b', lines[j]):
                body.append(lines[j].strip())
                j += 1
            pool[m.group(1)] = '|'.join(body)
            i = j
            continue
        out.append(lines[i])
        i += 1
    text = '\n'.join(out)
    text = re.sub(r'\bLC\d+\b', lambda m: '<' + pool.get(m.group(0), m.group(0)) + '>', text)
    # alignment directives that preceded a removed pool entry are not significant either
    text = re.sub(r'\n\t\.align 2(?=\n)', '', text)
    return text


a, b = canon(sys.argv[1]), canon(sys.argv[2])
if open(sys.argv[1]).read() == open(sys.argv[2]).read():
    print('IDENTICAL')
    sys.exit(0)
if a == b:
    print('EQUIVALENT modulo the string pool')
    sys.exit(1)
import difflib
for l in list(difflib.unified_diff(a.split('\n'), b.split('\n'), 'A', 'B', lineterm='', n=1))[:40]:
    print(l)
sys.exit(2)
