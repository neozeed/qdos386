#!/usr/bin/env python3
"""R16: mark an a.out executable as protected (CPL3) by setting bit 24 of the magic word (0x00000107 -> 0x01000107),
or clear it.   usage: r16-protect-mark.py IN [OUT]   (default OUT=IN; --clear to unset).  Prints both hashes."""
import hashlib, struct, sys
args = [a for a in sys.argv[1:] if a != "--clear"]
clear = "--clear" in sys.argv
src = args[0]; dst = args[1] if len(args) > 1 else args[0]
d = bytearray(open(src, "rb").read())
m = struct.unpack("<I", d[:4])[0]
base = m & ~0x01000000
if (base & 0xffff) != 0x107:
    sys.exit("not an a.out (magic %08x)" % m)
nm = base if clear else (base | 0x01000000)
d[:4] = struct.pack("<I", nm)
open(dst, "wb").write(d)
print("%s %08x -> %s %08x" % (src, m, dst, nm))
print("in  sha256 %s" % hashlib.sha256(open(src, "rb").read()).hexdigest())
print("out sha256 %s" % hashlib.sha256(d).hexdigest())
