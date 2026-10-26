#!/usr/bin/env python3
"""Build the R5 transfer area: a directory followed by exact a.out payloads.

Usage: make-xfer.py out.bin NAME=file.aout [NAME=file.aout ...]

Offsets are relative to XFER_BASE (0x280000).  The directory occupies the
first 128 bytes.  Each payload starts on a 16-byte boundary and is stored
byte-for-byte; the directory length is the exact file length, so padding is
never counted as file length.

Directory:  u32 magic 'QXFR', u32 version 1, u32 count, u32 0
            count x { u32 offset, u32 length, char name[4], u32 0 }
"""
import pathlib, struct, sys

DIR_BYTES = 128
MAX_IMAGES = 4
XFER_SIZE = 0x40000
MAGIC = 0x52465851           # 'Q','X','F','R' little endian

def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    out = pathlib.Path(sys.argv[1])
    specs = sys.argv[2:]
    if len(specs) > MAX_IMAGES:
        raise SystemExit('too many images (max %d)' % MAX_IMAGES)
    entries, blobs, off = [], [], DIR_BYTES
    for spec in specs:
        if '=' not in spec:
            raise SystemExit('expected NAME=file: ' + spec)
        name, path = spec.split('=', 1)
        if len(name) != 4:
            raise SystemExit('name must be 4 characters: ' + name)
        data = pathlib.Path(path).read_bytes()
        if not data:
            raise SystemExit('empty image: ' + path)
        entries.append((off, len(data), name.encode('ascii')))
        blobs.append(data)
        off = (off + len(data) + 15) & ~15
    if off > XFER_SIZE:
        raise SystemExit('transfer area overflow: %d bytes' % off)
    buf = bytearray(off)
    buf[0:16] = struct.pack('<IIII', MAGIC, 1, len(entries), 0)
    for i, (o, n, name) in enumerate(entries):
        buf[16 + 16 * i:32 + 16 * i] = struct.pack('<II4sI', o, n, name, 0)
    for (o, n, _), data in zip(entries, blobs):
        buf[o:o + n] = data
    out.write_bytes(bytes(buf))
    print('xfer: %d images, %d bytes' % (len(entries), len(buf)))

if __name__ == '__main__':
    main()
