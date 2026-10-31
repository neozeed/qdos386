#!/usr/bin/env python3
"""Remove named stub entries from src/stage3stubs.S.

Usage: unstub.py NAME [NAME ...]
Each stub is a block of the form
    \t.globl NAME
    NAME:
    \t<one or more instruction lines>
    <blank line>
The block is removed when NAME is listed. Prints the names it removed and the names it did not find.
"""
import re, sys
path = 'src/stage3stubs.S'
names = set(sys.argv[1:])
lines = open(path).read().split('\n')
out, removed, i = [], [], 0
while i < len(lines):
    m = re.match(r'^\s*\.globl\s+(\S+)\s*$', lines[i])
    if m and m.group(1) in names and i + 1 < len(lines) and lines[i + 1].startswith(m.group(1) + ':'):
        removed.append(m.group(1))
        i += 2
        # skip the instruction lines and the blank separator that follow
        while i < len(lines) and lines[i].strip() != '' and not re.match(r'^\s*\.globl', lines[i]):
            i += 1
        if i < len(lines) and lines[i].strip() == '':
            i += 1
        continue
    out.append(lines[i])
    i += 1
open(path, 'w').write('\n'.join(out))
print('removed:', ' '.join(removed))
missing = names - set(removed)
if missing:
    print('not found:', ' '.join(sorted(missing)))
