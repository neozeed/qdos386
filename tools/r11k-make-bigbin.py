#!/usr/bin/env python3
"""R11K: deterministic 192 KiB data file for the simultaneous-read tests.
   r11k-make-bigbin.py OUT      writes OUT and prints the checksum (hex) the TWICE program computes
   r11k-make-bigbin.py --sum-of FILE   prints that checksum for any file
Checksum: s = (s*31 + byte) mod 2^32 over all bytes, as in sdk/examples/r11k/twice.c."""
import sys

def csum(b):
    s = 0
    for x in b:
        s = (s * 31 + x) & 0xFFFFFFFF
    return s

if len(sys.argv) == 3 and sys.argv[1] == '--sum-of':
    print('%08x' % csum(open(sys.argv[2], 'rb').read()))
else:
    data = bytes(((i * 7) ^ (i >> 8) ^ (i >> 3)) & 0xFF for i in range(192 * 1024))
    open(sys.argv[1], 'wb').write(data)
    print('%08x' % csum(data))
