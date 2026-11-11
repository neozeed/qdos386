#!/usr/bin/env python3
# Apollo 16 Phase 2B: summarise the serial (debug-port) matrix logs into one table.
#   usage: python3 scripts/a16-p2b-serial-summary.py MATRIXDIR
# Each run directory holds serial.txt (debug-port bytes: NULs removed, lines de-duplicated).
import glob, os, re, sys

D = sys.argv[1]
PASS = re.compile(r"R20G (\w+) pass=(\d) calls=(\d+) bytes=(\d+) bad=(\d+) t0=(\d+) t1=(\d+) ticks=(\d+)")
STAT = re.compile(r"R20G (\w+) pass=(\d) hit=(-?\d+) miss=(-?\d+) fill=(-?\d+) restart=(-?\d+) coal=(-?\d+) "
                  r"evict=(-?\d+) nospace=(-?\d+) ide=(-?\d+) used=(-?\d+)")
PROG = re.compile(r"R20P progress calls=(\d+) tick=(\d+)")

def rows(path):
    raw = open(path, "rb").read().replace(b"\x00", b"").decode("latin-1")
    lines = sorted(set(l.strip() for l in raw.splitlines()))
    passes, stats, prog = {}, {}, []
    for l in lines:
        m = PASS.search(l)
        if m:
            passes[int(m.group(2))] = m.groups()
        m = STAT.search(l)
        if m:
            stats[int(m.group(2))] = m.groups()
        m = PROG.search(l)
        if m:
            prog.append((int(m.group(1)), int(m.group(2))))
    prog.sort(key=lambda x: x[1])
    return passes, stats, prog

def bg_rate(prog, t0, t1):
    if t1 <= t0:
        return None
    inside = [c for c, t in prog if t0 <= t <= t1]
    before = [c for c, t in prog if t <= t0]
    if not inside:
        return None
    return (inside[-1] - (before[-1] if before else 0)) / float(t1 - t0)

print("%-16s %-7s %4s %6s %5s %7s %7s %6s %6s %6s %6s %6s %6s" % (
    "run", "work", "pass", "ticks", "bad", "hit", "miss", "fill", "coal", "evict", "nosp", "ide", "bg/tick"))
for path in sorted(glob.glob(os.path.join(D, "*", "serial.txt"))):
    tag = os.path.basename(os.path.dirname(path))
    passes, stats, prog = rows(path)
    if not passes:
        print("%-16s NO RESULT" % tag)
        continue
    for p in sorted(passes):
        what, pn, calls, nbytes, bad, t0, t1, ticks = passes[p]
        st = stats.get(p)
        g = (lambda i: st[i]) if st else (lambda i: "-")
        bg = bg_rate(prog, int(t0), int(t1))
        print("%-16s %-7s %4s %6s %5s %7s %7s %6s %6s %6s %6s %6s %6s" % (
            tag, what, pn, ticks, bad,
            g(2), g(3), g(4), g(6), g(7), g(8), g(9),
            "%.0f" % bg if bg is not None else "-"))
