#!/usr/bin/env python3
"""R13A: does a binary contain Minerva 1.98 font bitmaps?  The glyph rows are read from the Minerva GAS source
modules (third_party/minerva/src/tb_f0.s, tb_f1.s).  For each binary it reports:
  * table runs: the longest run of CONSECUTIVE Minerva glyphs (code order, starting with two glyphs of at least 3 set
    rows each) found contiguously - in the QL layout
    (pixels in bits 6..2), the R13 kernel layout (shifted left by one) and the QFNT layout (5-bit rows).  A copy of
    the font, or of a part of it, shows as a run >= 2.
  * single-glyph coincidences: every glyph with at least 3 set rows whose 9 bytes occur anywhere, and whether the
    match lies inside an --allow'ed font image (e.g. the public-domain built-in fonts, whose simple glyphs such as
    '+', ':', '{' can be pixel-identical to Minerva's).
  usage: scan-minerva.py TB_F0.s TB_F1.s [--allow FILE.FNT ...] BINARY...
  exit 1 if any binary has a run >= 2 or a match outside the allowed images."""
import re, sys


def rows(path):
    out, first = [], None
    for line in open(path, encoding="latin-1"):
        m = re.match(r"^/\* QL:\s+dc\.b\s+(\S+)", line)
        if not m:
            continue
        t = m.group(1)
        if first is None:
            first = int(t[1:3], 16)
            continue
        out.append(sum(0x40 >> i for i, ch in enumerate(t) if ch == "O"))
    return [out[9 * i:9 * i + 9] for i in range(len(out) // 9)]


a = sys.argv[1:]
glyphs = rows(a[0]) + rows(a[1])
a = a[2:]
allow, bins = [], []
while a:
    if a[0] == "--allow":
        allow.append(open(a[1], "rb").read())
        a = a[2:]
    else:
        bins.append(a[0])
        a = a[1:]
layouts = {"ql": lambda b: b, "r13": lambda b: (b << 1) & 0xFF, "qfnt": lambda b: (b >> 2) & 0x1F}
bad = 0
for p in bins:
    d = open(p, "rb").read()
    regions = []
    for img in allow:
        k = d.find(img)
        if k >= 0:
            regions.append((k, k + len(img)))
    best = 0
    for name, f in layouts.items():
        enc = [bytes(f(b) for b in g) for g in glyphs]
        dense = [sum(1 for b in g if b) >= 3 for g in glyphs]
        for i in range(len(enc) - 1):
            if not (dense[i] and dense[i + 1]):
                continue                            # blank / sparse glyphs (space, '.', '-') match unrelated bytes
            k = d.find(enc[i] + enc[i + 1])
            if k >= 0:
                n = 2
                while i + n < len(enc) and d[k + 9 * n:k + 9 * n + 9] == enc[i + n]:
                    n += 1
                best = max(best, n)
    inside, outside = [], []
    for i, g in enumerate(glyphs):
        if sum(1 for b in g if b) < 3:
            continue
        for name, f in layouts.items():
            pat = bytes(f(b) for b in g)
            k = d.find(pat)
            while k >= 0:
                (inside if any(lo <= k < hi for lo, hi in regions) else outside).append((i, name, k))
                k = d.find(pat, k + 1)
    ok = best < 2 and not outside
    bad |= not ok
    print("%s: %s - longest run of consecutive Minerva glyphs %d; single-glyph coincidences: %d inside allowed font "
          "images, %d elsewhere%s" % (p, "NO MINERVA TABLE" if ok else "MINERVA DATA PRESENT", best, len(inside),
                                       len(outside), (" " + str(outside[:6])) if outside else ""))
sys.exit(1 if bad else 0)
