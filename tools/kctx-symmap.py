#!/usr/bin/env python3
"""KCTX-FIX1: address-sorted symbol map of a kernel a.out (build/*/kernel.aout), with the object each symbol came from
when the a.out keeps file-name symbols (GAS/ld N_FN entries).  Used as the link map of the delivered kernels.
   usage: python3 tools/kctx-symmap.py kernel.aout > kernel.map"""
import struct, sys
d = open(sys.argv[1], "rb").read()
mag, text, data, bss, syms, entry, trs, drs = struct.unpack("<8I", d[:32])
so = 32 + text + data + trs + drs; st = so + syms
kinds = {0x02: "A", 0x04: "T", 0x06: "D", 0x08: "B", 0x1e: "F", 0x1f: "F"}
rows = []
for k in range(0, syms, 12):
    sx, ty, _o, _d, val = struct.unpack("<IBBHI", d[so + k:so + k + 12])
    if sx == 0: continue
    nm = d[st + sx:d.index(b"\0", st + sx)].decode("latin-1")
    t = kinds.get(ty & 0x1e if ty not in (0x1e, 0x1f) else ty, "?")
    if t == "?": continue
    rows.append((val, ("%s" % t) if ty & 1 else t.lower() if t != "F" else "F", nm))
print("# %s: magic %o text %d data %d bss %d entry 0x%x" % (sys.argv[1], mag & 0xffff, text, data, bss, entry))
for val, t, nm in sorted(rows, key=lambda r: (r[0], r[2])):
    print("%08x %s %s" % (val, t, nm))
