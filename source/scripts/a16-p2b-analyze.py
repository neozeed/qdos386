#!/usr/bin/env python3
# Apollo 16 Phase 2B: summarise the matrix transcripts (R20G pass lines, R20P progress, LEAK heap) into a table.
#   usage: python3 scripts/a16-p2b-analyze.py MATRIXDIR > summary.txt
# Transcripts repeat screen snapshots, so every line is de-duplicated (sort -u) before the numbers are read.
import glob, os, re, sys

D = sys.argv[1]
PASS = re.compile(r"R20G (\w+) pass=(\d) calls=(\d+) bytes=(\d+) bad=(\d+) t0=(\d+) t1=(\d+) ticks=(\d+)")
STAT = re.compile(r"R20G (\w+) pass=(\d)((?: \w+=-?\d+)+)")
PROG = re.compile(r"R20P progress calls=(\d+) tick=(\d+)")
LEAK = re.compile(r"LEAK heap=(\d+)")

def load(path):
    lines = sorted(set(l.rstrip() for l in open(path, errors="replace")))
    return lines

def bg_rate(prog, t0, t1):
    inside = [(c, t) for c, t in prog if t0 <= t <= t1]
    before = [(c, t) for c, t in prog if t <= t0]
    if not inside or t1 <= t0:
        return None
    c1, _ = inside[-1]
    c0 = before[-1][0] if before else 0
    return (c1 - c0) / float(t1 - t0)

print("%-16s %-6s %-5s %9s %7s %4s %7s %7s %7s %6s %6s %5s %6s %5s %5s %6s" % (
    "run", "work", "pass", "calls", "ticks", "bad", "hit", "miss", "fill", "coal", "evict", "nosp", "ide", "rst", "used", "bg/tick"))
for path in sorted(glob.glob(os.path.join(D, "qemu-*.txt"))):
    tag = os.path.basename(path)[len("qemu-"):-len(".txt")]
    lines = load(path)
    prog = sorted(set((int(m.group(1)), int(m.group(2))) for l in lines for m in [PROG.search(l)] if m), key=lambda x: x[1])
    passes = {}
    for l in lines:
        m = PASS.search(l)
        if m:
            passes.setdefault((m.group(1), int(m.group(2))), {}).update(
                calls=int(m.group(3)), bytes=int(m.group(4)), bad=int(m.group(5)),
                t0=int(m.group(6)), t1=int(m.group(7)), ticks=int(m.group(8)))
        s = STAT.search(l)
        if s:
            kv = dict(x.split("=") for x in s.group(3).split())
            passes.setdefault((s.group(1), int(s.group(2))), {}).update({k: int(v) for k, v in kv.items()})
    if not passes:
        print("%-16s (no pass lines)" % tag)
        continue
    for (work, p), r in sorted(passes.items()):
        rate = bg_rate(prog, r.get("t0", 0), r.get("t1", 0)) if "t0" in r else None
        def g(k):
            v = r.get(k)
            return "-" if v is None or v < 0 else str(v)
        print("%-16s %-6s %-5s %9s %7s %4s %7s %7s %7s %6s %6s %5s %6s %5s %5s %6s" % (
            tag, work, p, g("calls"), g("ticks"), g("bad"), g("hit"), g("miss"), g("fill"), g("coal"), g("evict"),
            g("nospace"), g("ide"), g("restart"), g("used"), "-" if rate is None else "%.0f" % rate))
    heap = [int(m.group(1)) for l in lines for m in [LEAK.search(l)] if m]
    if heap:
        print("%-16s heap first=%d last=%d (LEAK, qd_free_bytes)" % (tag, heap[0], heap[-1]))
