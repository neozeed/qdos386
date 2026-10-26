#!/usr/bin/env python3
"""Build a Tiny386 raw disk image: boot sector followed by the kernel.

Usage: make-image.py boot.bin kernel.raw out.img
The kernel is padded to whole sectors.  The sector count is written into
the boot sector word at offset 0x1C2 (see boot.s).
"""
import pathlib, sys
boot_p, kern_p, out_p = map(pathlib.Path, sys.argv[1:4])
boot = bytearray(boot_p.read_bytes())
kern = kern_p.read_bytes()
if len(boot) != 512 or boot[0x1FE:0x200] != b'\x55\xaa':
    raise SystemExit('bad boot sector')
sectors = (len(kern) + 511) // 512
if sectors == 0 or sectors > 127:
    raise SystemExit('kernel must be 1..127 sectors (got %d)' % sectors)
if sectors * 512 > 0xF000:
    raise SystemExit('kernel exceeds one 64K staging buffer')
boot[0x1C2:0x1C4] = sectors.to_bytes(2, 'little')
out_p.write_bytes(bytes(boot) + kern.ljust(sectors * 512, b'\0'))
print('kernel bytes=%d sectors=%d image=%d' % (len(kern), sectors, 512 + sectors * 512))
