#!/usr/bin/env python3
"""Build the R6 WIN1_ FAT16 volume: a 3 MiB image with 512-byte sectors and
one sector per cluster, two FAT copies, a 512-entry root directory, and the
given files in the root directory.

Usage: make-fat16.py out.img NAME.EXT=path [NAME.EXT=path ...]

The volume is FAT16 by construction: 6144 sectors give about 6000 clusters,
inside the 4085..65524 range that the kernel uses to classify the volume.
The same bytes go on the Tiny386 hdb disk and into the i386VM image.
"""
import hashlib, pathlib, re, sys

TOTAL = 6144                 # sectors: 3 MiB, the size of the reserved RAM disk
BPS = 512
RESERVED = 1
NFATS = 2
ROOT_ENTRIES = 512
ROOT_SECTORS = ROOT_ENTRIES * 32 // BPS
NAME_RE = re.compile(r'^[A-Z0-9_]{1,8}\.[A-Z0-9_]{1,3}$')

def sectors_per_fat(clusters):
    spf = 1
    while spf * (BPS // 2) < clusters + 2:
        spf += 1
    return spf

def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    out = pathlib.Path(sys.argv[1])
    files = []
    for arg in sys.argv[2:]:
        name, _, path = arg.partition('=')
        if not NAME_RE.match(name):
            raise SystemExit('not an uppercase 8.3 name: %r' % name)
        data = pathlib.Path(path).read_bytes()
        if len(data) == 0:
            raise SystemExit('empty file: %s' % name)
        files.append((name, data))
    if len(files) > ROOT_ENTRIES:
        raise SystemExit('too many files')

    spf = sectors_per_fat(TOTAL - RESERVED - NFATS * 1 - ROOT_SECTORS)
    while True:
        data_sectors = TOTAL - RESERVED - NFATS * spf - ROOT_SECTORS
        clusters = data_sectors          # one sector per cluster
        if spf * (BPS // 2) >= clusters + 2:
            break
        spf += 1
    if not (4085 <= clusters <= 65524):
        raise SystemExit('cluster count %d is not FAT16' % clusters)

    img = bytearray(TOTAL * BPS)
    # Boot sector / BPB
    b = img
    b[0:3] = b'\xeb\x3c\x90'
    b[3:11] = b'QDOS386 '
    b[11:13] = BPS.to_bytes(2, 'little')
    b[13] = 1
    b[14:16] = RESERVED.to_bytes(2, 'little')
    b[16] = NFATS
    b[17:19] = ROOT_ENTRIES.to_bytes(2, 'little')
    b[19:21] = TOTAL.to_bytes(2, 'little')
    b[21] = 0xf8
    b[22:24] = spf.to_bytes(2, 'little')
    b[24:26] = (63).to_bytes(2, 'little')
    b[26:28] = (255).to_bytes(2, 'little')
    b[28:32] = (0).to_bytes(4, 'little')
    b[32:36] = (0).to_bytes(4, 'little')
    b[36] = 0x80
    b[38] = 0x29
    b[39:43] = (0x51444f53).to_bytes(4, 'little')
    b[43:54] = b'WIN1       '
    b[54:62] = b'FAT16   '
    b[510] = 0x55
    b[511] = 0xaa

    # FAT: entries 0 and 1 reserved; each file gets a contiguous run of clusters.
    fat = bytearray(spf * BPS)
    fat[0:4] = b'\xf8\xff\xff\xff'
    root = bytearray(ROOT_SECTORS * BPS)
    data_start = RESERVED + NFATS * spf + ROOT_SECTORS
    data = bytearray(clusters * BPS)
    next_cluster = 2
    for i, (name, content) in enumerate(files):
        n = (len(content) + BPS - 1) // BPS
        first = next_cluster
        if first + n - 1 > clusters + 1:
            raise SystemExit('files do not fit in the volume')
        for k in range(n):
            c = first + k
            nxt = c + 1 if k + 1 < n else 0xffff
            fat[2 * c:2 * c + 2] = nxt.to_bytes(2, 'little')
            chunk = content[k * BPS:(k + 1) * BPS]
            off = (c - 2) * BPS
            data[off:off + len(chunk)] = chunk
        e = bytearray(32)
        base, ext = name.split('.')
        e[0:11] = (base.ljust(8) + ext.ljust(3)).encode('ascii')
        e[11] = 0x20                                  # archive
        e[26:28] = first.to_bytes(2, 'little')
        e[28:32] = len(content).to_bytes(4, 'little')
        root[32 * i:32 * i + 32] = e
        next_cluster = first + n

    fat_block = bytes(fat)
    img[RESERVED * BPS:(RESERVED + spf) * BPS] = fat_block
    img[(RESERVED + spf) * BPS:(RESERVED + 2 * spf) * BPS] = fat_block
    img[(RESERVED + 2 * spf) * BPS:data_start * BPS] = root
    img[data_start * BPS:data_start * BPS + len(data)] = data

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(bytes(img))
    digest = hashlib.sha256(bytes(img)).hexdigest()
    print('win1 fat16: sectors=%d spf=%d clusters=%d data_start=%d files=%d sha256=%s' %
          (TOTAL, spf, clusters, data_start, len(files), digest))

if __name__ == '__main__':
    main()
