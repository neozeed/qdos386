#!/usr/bin/env python3
"""R11K: scan a.out programs for what a paged launch relies on.  For every file: magic, text/data/bss
sizes, entry, relocation sizes (must be 0), whether the entry is the slot-1 origin 0x240000, and a
(with --consts, a weak heuristic: every SDK program shows dozens of unrelated hits, so it proves nothing) byte-wise scan of text+data for 32-bit constants inside the profile 2 / profile 3 spans
(0x380000..0x47FFFF, 0x500000..0x5FFFFF) or the slot 0 / slot 1 windows' outer neighbours, which would
indicate embedded assumptions about other load addresses.  usage: r11k-aout-scan.py FILE...
"""
import struct, sys
CONSTS = "--consts" in sys.argv
sys.argv = [a for a in sys.argv if a != "--consts"]
SPANS = {"profile2": (0x380000, 0x480000), "profile3": (0x500000, 0x600000), "slot0": (0x200000, 0x240000),
         "frames-vm": (0x900000, 0x1000000)}
rows = []
for path in sys.argv[1:]:
    b = open(path, "rb").read()
    if len(b) < 32:
        rows.append((path, "short")); continue
    magic, text, data, bss, syms, entry, tr, dr = struct.unpack_from("<8I", b)
    body = b[32:32 + text + data]
    hits = {k: 0 for k in SPANS}
    for i in range(0, (len(body) - 3) if CONSTS else 0):
        v = int.from_bytes(body[i:i + 4], "little")
        for k, (lo, hi) in SPANS.items():
            if lo <= v < hi:
                hits[k] += 1
    rows.append((path, magic, text, data, bss, entry, tr, dr, hits))
for r in rows:
    if len(r) == 2:
        print(r[0], r[1]); continue
    path, magic, text, data, bss, entry, tr, dr, hits = r
    flag = "OK" if (magic == 0o407 and tr == 0 and dr == 0 and entry == 0x240000) else "CHECK"
    print("%-6s %-34s magic %o text %6d data %5d bss %6d entry %06x reloc %d/%d  consts: %s" % (
        flag, path.split("/")[-1], magic, text, data, bss, entry, tr, dr,
        ",".join("%s=%d" % (k, v) for k, v in hits.items() if v) or "none"))
