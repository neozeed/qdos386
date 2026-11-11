#!/usr/bin/env python3
"""i386VM flat image: kernel raw bytes, zero padding to XFER_BASE, transfer
area, then (R6) the WIN1_ FAT16 volume at RAMDISK_BASE.

Usage: make-vm-image.py kernel.raw xfer.bin out.bin [win1.img]

The harness loads the file at guest address 0, so each region sits at its
guest address: transfer at 0x280000 (padded to XFER_SIZE), the RAM disk at
0x2c0000.  Padding is zeros and is never read as data (the directory and the
FAT give all lengths).
"""
import os, pathlib, sys

# QDOS_LAYOUT=r9 selects the R9 map (see kernel/appmap.h); default is R5-R8.
if os.environ.get('QDOS_LAYOUT', '') == 'r9':
    XFER_BASE = 0x340000
    RAMDISK_BASE = 0x900000
else:
    XFER_BASE = 0x280000
    RAMDISK_BASE = 0x2c0000
XFER_SIZE = 0x40000
RAMDISK_SIZE = 0x300000

if len(sys.argv) not in (4, 5):
    raise SystemExit(__doc__)
kern = pathlib.Path(sys.argv[1]).read_bytes()
xfer = pathlib.Path(sys.argv[2]).read_bytes()
disk = pathlib.Path(sys.argv[4]).read_bytes() if len(sys.argv) == 5 else b''
if len(kern) > XFER_BASE:
    raise SystemExit('kernel overlaps the transfer area')
if len(xfer) > XFER_SIZE:
    raise SystemExit('transfer area too large')
if disk and len(disk) != RAMDISK_SIZE:
    raise SystemExit('RAM disk must be exactly %d bytes, got %d' % (RAMDISK_SIZE, len(disk)))
out = kern + b'\0' * (XFER_BASE - len(kern)) + xfer.ljust(XFER_SIZE, b'\0')
if disk:
    # The RAM disk sits at its guest address; the gap is zero padding.
    out += b'\0' * (RAMDISK_BASE - len(out)) + disk
pathlib.Path(sys.argv[3]).write_bytes(out)
print('vm image: kernel=%d xfer@0x%x disk@0x%x total=%d' %
      (len(kern), XFER_BASE, RAMDISK_BASE if disk else 0, len(out)))
