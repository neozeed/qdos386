#!/usr/bin/env python3
"""KCTX-FIX2: which resume convention a LINKED kernel really contains, read from its bytes (not from the Makefile).
Finds resume_dispatch in the a.out symbol table and decodes its first instructions in kernel.raw (loaded at 0x100000):
  exact    movl %eax,%esp ; cmpl $0,_kx_resume_fmt      (89 c4 83 3d <&kx_resume_fmt> 00)  - kernel/kctx.s
  ptrbits  movl %eax,%ecx ; andl $1,%ecx ; andl $-4,%eax (89 c1 83 e1 01 83 e0 fc)          - frozen / context.s
   usage: python3 tools/kctx2-resume-check.py kernel.aout kernel.raw [expect: exact|ptrbits]   (exit 1 on mismatch)"""
import struct, sys
a = open(sys.argv[1], "rb").read(); raw = open(sys.argv[2], "rb").read()
expect = sys.argv[3] if len(sys.argv) > 3 else None
_m, text, data, bss, syms, _e, trs, drs = struct.unpack("<8I", a[:32])
so = 32 + text + data + trs + drs; st = so + syms
sym = {}
for k in range(0, syms, 12):
    sx, ty, _o, _d, val = struct.unpack("<IBBHI", a[so + k:so + k + 12])
    if sx:
        sym.setdefault(a[st + sx:a.index(b"\0", st + sx)].decode("latin-1"), val)
BASE = 0x100000
if "resume_dispatch" not in sym:
    print("no resume_dispatch symbol (not a DYNPROC kernel)"); sys.exit(2)
rd = sym["resume_dispatch"]; b = raw[rd - BASE:rd - BASE + 12]
fmt = sym.get("_kx_resume_fmt")
if b[:4] == bytes.fromhex("89c4833d") and fmt is not None and struct.unpack("<I", b[4:8])[0] == fmt and b[8] == 0:
    kind = "exact"; info = "movl %%eax,%%esp; cmpl $0,_kx_resume_fmt (0x%x)" % fmt
elif b[:8] == bytes.fromhex("89c183e10183e0fc"):
    kind = "ptrbits"; info = "movl %eax,%ecx; andl $1,%ecx; andl $-4,%eax (format in bit 0, ESP masked)"
else:
    kind = "unknown"; info = b.hex()
print("%s: resume_dispatch at 0x%x: %s -> %s" % (sys.argv[1], rd, info, kind.upper()))
sys.exit(0 if expect in (None, kind) else 1)
