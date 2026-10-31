#!/usr/bin/env python3
"""Summarise a QEMU VGA capture (scripts/r11m-qemu-capture.py lines.txt): for every "=== CMD" marker print the
non-empty rows of the LAST screen snapshot taken before the next marker.  usage: r11m-qlast.py lines.txt [CMD-substring ...]"""
import sys
seg, cur, snap, name = [], None, [], None
out = []
def flush():
    if name is not None:
        out.append((name, [r for r in snap if r.strip()]))
for line in open(sys.argv[1], errors="replace").read().split("\n"):
    if line.startswith("=== CMD "):
        flush(); name, snap = line[8:], []
    elif line.startswith("=== SNAP"):
        snap = []
    elif line.startswith("=== "):
        snap.append(line)
    else:
        snap.append(line)
flush()
filt = sys.argv[2:]
for n, rows in out:
    if filt and not any(f in n for f in filt):
        continue
    print("### " + n)
    for r in rows:
        print("   " + r)
