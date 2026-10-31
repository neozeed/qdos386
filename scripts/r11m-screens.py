#!/usr/bin/env python3
"""R11M: turn a QEMU VGA capture (scripts/r11m-qemu-capture.py transcript) into a readable "screens" file: for every typed command the LAST
80x25 snapshot taken before the next command.  usage: r11m-screens.py qemu-a.txt > demo-screens.txt"""
import sys

t = open(sys.argv[1], errors="replace").read().split("=== CMD ")
print("QEMU VGA text-buffer snapshots, one per command (the screen after the command finished)\n")
for seg in t[1:]:
    head, _, body = seg.partition("\n")
    snaps = body.split("=== SNAP ")
    if len(snaps) < 2:
        print("#### %s\n(no screen change)\n" % head)
        continue
    last = snaps[-1].split("\n", 1)[1] if "\n" in snaps[-1] else ""
    rows = [r for r in last.split("\n") if r.strip()]
    print("#### %s" % head)
    print("+" + "-" * 80 + "+")
    for r in rows[-25:]:
        print("|" + r[:80].ljust(80) + "|")
    print("+" + "-" * 80 + "+\n")
