#!/usr/bin/env python3
"""R11J: extract the regular root files of a QDOS FAT16 volume, in directory order, with their
exact bytes.  The three fixture entries make-fat16.py always adds (SUBDIR, BAD*NAME.TXT,
CHAIN.AOU) are skipped: the tool recreates them.  Writes the files into OUTDIR and prints one
NAME=OUTDIR/NAME argument per file, in order, for make-fat16.py.
usage: r11j-extract-volume.py VOLUME OUTDIR"""
import pathlib, struct, sys

FIXTURES = {'SUBDIR', 'BAD*NAME.TXT', 'CHAIN.AOU'}

def main():
    img, outdir = sys.argv[1], pathlib.Path(sys.argv[2])
    d = pathlib.Path(img).read_bytes()
    bps, spc, res, nf, rent = struct.unpack_from('<HBHBH', d, 11)
    spf = struct.unpack_from('<H', d, 22)[0]
    root = (res + nf * spf) * bps
    data = root + ((rent * 32 + bps - 1) // bps) * bps
    fat = res * bps
    outdir.mkdir(parents=True, exist_ok=True)
    for i in range(rent):
        e = d[root + 32 * i: root + 32 * i + 32]
        if e[0] == 0:
            break
        if e[0] == 0xE5 or e[11] & 0x18:
            continue
        base, ext = e[0:8].decode().rstrip(), e[8:11].decode().rstrip()
        name = base + ('.' + ext if ext else '')
        if name in FIXTURES:
            continue
        cl = struct.unpack_from('<H', e, 26)[0]
        size = struct.unpack_from('<I', e, 28)[0]
        b = b''
        while 2 <= cl < 0xFFF8 and len(b) < size:
            off = data + (cl - 2) * spc * bps
            b += d[off: off + spc * bps]
            cl = struct.unpack_from('<H', d, fat + 2 * cl)[0]
        if len(b) < size:
            raise SystemExit('short chain: ' + name)
        (outdir / name).write_bytes(b[:size])
        print('%s=%s' % (name, outdir / name))

if __name__ == '__main__':
    main()
