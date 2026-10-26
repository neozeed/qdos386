#!/usr/bin/env python3
"""Build a Tiny386 raw disk image: boot sector, kernel, transfer area.

Usage: make-image.py boot.bin kernel.raw xfer.bin out.img
Sector 1..K holds the kernel, sectors K+1..K+X hold the transfer area.  The
boot sector reads both with one INT 13h call (see boot.s).  Counts are written
at offsets 0x1C2 (K) and 0x1C4 (X).
"""
import pathlib, sys
boot_p, kern_p, xfer_p, out_p = map(pathlib.Path, sys.argv[1:5])
boot = bytearray(boot_p.read_bytes())
kern = kern_p.read_bytes()
xfer = xfer_p.read_bytes()
if len(boot) != 512 or boot[0x1FE:0x200] != b'\x55\xaa':
    raise SystemExit('bad boot sector')
ksec = (len(kern) + 511) // 512
xsec = (len(xfer) + 511) // 512
if ksec == 0 or ksec > 127:
    raise SystemExit('kernel must be 1..127 sectors (got %d)' % ksec)
if xsec == 0 or xsec > 0x40000 // 512:
    raise SystemExit('transfer area must be 1..512 sectors (got %d)' % xsec)
if (ksec + xsec) * 512 > 0xF000:
    raise SystemExit('kernel + transfer exceed the 64K real-mode buffer')
boot[0x1C2:0x1C4] = ksec.to_bytes(2, 'little')
boot[0x1C4:0x1C6] = xsec.to_bytes(2, 'little')
out_p.write_bytes(bytes(boot) + kern.ljust(ksec * 512, b'\0') + xfer.ljust(xsec * 512, b'\0'))
print('kernel bytes=%d sectors=%d transfer bytes=%d sectors=%d' % (len(kern), ksec, len(xfer), xsec))
