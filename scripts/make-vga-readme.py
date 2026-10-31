#!/usr/bin/env python3
"""Write the 63-line README.TXT used on the R10-VGA and R11A test volumes.
CRLF line endings.  usage: make-vga-readme.py OUTFILE"""
import sys
lines = ["QDOS/386 R10-VGA test file README.TXT",
         "This file is long on purpose: TYPE shows it, and the screen scrolls.",
         ""]
for i in range(1, 61):
    lines.append("line %02d: the quick brown fox jumps over the lazy dog 0123456789" % i)
with open(sys.argv[1], "wb") as f:
    f.write(("\r\n".join(lines) + "\r\n").encode("ascii"))
