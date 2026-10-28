#!/usr/bin/env python3
"""R11M: summarize the final evidence directory (the output of scripts/r11m-matrix.sh rounds, r11m-regress.sh, host tests).
usage: r11m-summary.py EVIDENCE_DIR > SUMMARY.txt   (exit 1 if anything failed or is missing)"""
import glob
import os
import re
import sys

D = sys.argv[1]
bad = 0


def line(s):
    print(s)


def last(path):
    try:
        t = [l for l in open(path, errors="replace").read().splitlines() if l.strip()]
        return t[-1] if t else ""
    except OSError:
        return ""


line("R11M evidence summary (%s)" % os.path.basename(os.path.abspath(D)))
line("")
line("== core matrix, three rounds per emulator (scripts/r11m-matrix.sh, judged by scripts/r11m-matrix-judge.py)")
for emu in ("vm", "pc", "qemu"):
    for r in (1, 2, 3):
        if os.path.exists(os.path.join(D, "%s-r%d.ABORTED" % (emu, r))):
            line("  %-5s round %d: ABORTED by decision before completion (cases that ran: see %s-r%d.judge.txt; NOT counted)" % (emu, r, emu, r))
            continue
        j = os.path.join(D, "%s-r%d.judge.txt" % (emu, r))
        s = last(j)
        ok = s.startswith("MATRIX %s PASS" % emu)
        if not ok:
            bad += 1
        line("  %-5s round %d: %s" % (emu, r, s or "MISSING"))
line("")
line("== sub-gates inside every round (VFAT gate, integrated demonstration)")
for emu in ("vm", "pc", "qemu"):
    for r in (1, 2, 3):
        if os.path.exists(os.path.join(D, "%s-r%d.ABORTED" % (emu, r))):
            continue
        g = last(os.path.join(D, "%s-r%d" % (emu, r), "%s-vfat.txt" % emu))
        d = last(os.path.join(D, "%s-r%d" % (emu, r), "%s-demo.txt" % emu))
        ok = g == "GATE %s PASS" % emu and d == "DEMO-ALL %s PASS" % emu
        if not ok:
            bad += 1
        line("  %-5s round %d: %s | %s" % (emu, r, g or "MISSING", d or "MISSING"))
line("")
line("== regression of the R11K/R11H/R11F sequences on the R11M kernels (scripts/r11m-regress.sh, 3 rounds, 3 QEMU rounds)")
f = os.path.join(D, "regress.filter.txt")
for l in open(f, errors="replace").read().splitlines() if os.path.exists(f) else ["MISSING regress.filter.txt"]:
    line("  " + l)
    if l.startswith("real failures:") and l.strip() != "real failures: 0":
        bad += 1
neg = 0
negok = 0
for p in glob.glob(os.path.join(D, "regress", "*neg*.txt")):
    if p.endswith("-input.txt"):
        continue
    t = open(p, errors="replace").read()
    m = re.search(r"NEG: checks (\d+), failures (\d+)", t)
    if m:
        neg += 1
        negok += (m.group(1) == "848" and m.group(2) == "0")
line("  NEG transcripts with 'checks 848, failures 0': %d of %d" % (negok, neg))
if neg == 0 or negok != neg:
    bad += 1
vol = last(os.path.join(D, "regress.log"))
rl = open(os.path.join(D, "regress.log"), errors="replace").read() if os.path.exists(os.path.join(D, "regress.log")) else ""
line("  frozen R11K/R11H volumes: %s" % ("UNCHANGED" if "VOLUMES UNCHANGED" in rl else "CHANGED or unknown"))
if "VOLUMES UNCHANGED" not in rl:
    bad += 1
line("")
line("== host tests (tests/host/r11m/run.sh)")
h = os.path.join(D, "hosttests.log")
ht = open(h, errors="replace").read() if os.path.exists(h) else ""
for l in ht.splitlines():
    if "PASS" in l or l.startswith("EXIT"):
        line("  " + l)
if "R11M host tests: PASS" not in ht or "EXIT 0" not in ht:
    bad += 1
line("")
line("== frozen default images")
h = os.path.join(D, "default-hashes.txt")
dh = open(h, errors="replace").read() if os.path.exists(h) else ""
for l in dh.splitlines():
    line("  " + l)
if dh.count("IDENTICAL") != 6:
    bad += 1
line("")
line("OVERALL: %s" % ("PASS" if bad == 0 else "FAIL (%d problems)" % bad))
sys.exit(1 if bad else 0)
