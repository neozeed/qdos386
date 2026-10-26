#!/usr/bin/env python3
"""i386VM flat image: kernel raw bytes, zero padding to XFER_BASE, transfer area.

Usage: make-vm-image.py kernel.raw xfer.bin out.bin
The harness loads the file at guest address 0, so the transfer area sits at
file offset 0x280000 = guest address 0x280000.  The padding is zeros and is
never read as application data (the directory gives all lengths).
"""
import pathlib, sys

XFER_BASE = 0x280000
XFER_SIZE = 0x40000

kern = pathlib.Path(sys.argv[1]).read_bytes()
xfer = pathlib.Path(sys.argv[2]).read_bytes()
if len(kern) > XFER_BASE:
    raise SystemExit('kernel overlaps the transfer area')
if len(xfer) > XFER_SIZE:
    raise SystemExit('transfer area too large')
out = kern + b'\0' * (XFER_BASE - len(kern)) + xfer
pathlib.Path(sys.argv[3]).write_bytes(out)
print('vm image: kernel=%d pad=%d xfer=%d total=%d' % (len(kern), XFER_BASE - len(kern), len(xfer), len(out)))
