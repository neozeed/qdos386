#!/usr/bin/env python3
"""Deterministic fixture files for the WIN1_ volume (R7).

  big.bin    8200 bytes: larger than the 8 KiB executable cap, so RUN must
             refuse it before parsing.
  bad.bin    64 bytes that are not an a.out (wrong magic): RUN must report a
             format error.
  empty.bin  zero bytes: EMPTY.TXT, listed with size 0.

Usage: make-fixtures.py outdir
"""
import pathlib, sys

def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    out = pathlib.Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    big = bytes((i * 7 + 3) & 0xff for i in range(8200))
    bad = bytearray(64)
    bad[0:4] = b'NOPE'
    (out / 'big.bin').write_bytes(big)
    (out / 'bad.bin').write_bytes(bytes(bad))
    (out / 'empty.bin').write_bytes(b'')

if __name__ == '__main__':
    main()
