#!/usr/bin/env python3
"""R11K investigation tool: build a disk image for the ORIGINAL (R11J) boot sector with an arbitrary
total sector count, bypassing platform/pc/make-image.py's checks, to see where the legacy loader
really stops.  The kernel is zero-padded after its last byte (that area is BSS, cleared by kstart),
so a successful boot only shows that the loader read and copied the bytes; it is not a kernel-size
claim.  usage: r11k-oldlimit-image.py boot.bin kernel.raw xfer.bin total_sectors out.img
"""
import sys, pathlib
boot, kern, xfer = (pathlib.Path(a).read_bytes() for a in sys.argv[1:4])
total = int(sys.argv[4]); out = pathlib.Path(sys.argv[5])
xsec = (len(xfer) + 511) // 512
ksec = total - xsec
assert ksec * 512 >= len(kern), "total too small for the kernel"
b = bytearray(boot)
b[0x1C2:0x1C4] = ksec.to_bytes(2, 'little'); b[0x1C4:0x1C6] = xsec.to_bytes(2, 'little')
out.write_bytes(bytes(b) + kern.ljust(ksec * 512, b'\0') + xfer.ljust(xsec * 512, b'\0'))
print("total", total, "ksec", ksec, "xsec", xsec)
