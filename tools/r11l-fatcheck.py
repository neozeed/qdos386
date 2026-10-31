#!/usr/bin/env python3
"""R11L independent FAT16 checker, manifest writer, extractor and repair tool.

It shares no code with the kernel (kernel/fat16.c) and is not a port of it.  The strict rules it
enforces are the ones the guest driver enforces when it opens a file, plus the usual fsck checks:

  ERRORS  (volume is inconsistent)
    FAT copy mismatch; cluster number out of range; cycle; chain not ending in an end marker;
    a chain that reaches a free (0) or bad (0xFFF7) entry; two files sharing a cluster;
    chain length != ceil(size / cluster bytes) (longer or shorter); size 0 with a first cluster;
    size > 0 with first cluster 0; duplicate names; invalid 8.3 name in a file entry.
  WARNINGS (volume is consistent but wastes space)
    orphan clusters: allocated (not 0, not bad) but reachable from no root entry.

Usage:
  r11l-fatcheck.py IMG                  check; exit 0 consistent (warnings allowed), 1 errors, 2 usage/IO
  r11l-fatcheck.py IMG --strict         warnings count as errors
  r11l-fatcheck.py IMG --manifest       also print "NAME SIZE SHA256" per file, sorted by name
  r11l-fatcheck.py IMG --extract DIR    write every file into DIR
  r11l-fatcheck.py IMG --repair         fix what can be fixed IN PLACE (copy the image first):
                                        FAT2 := FAT1; chain longer than size -> free the tail;
                                        chain shorter than size -> size := chain capacity (or the
                                        largest multiple that is valid); orphans -> freed.
"""
import hashlib, os, re, struct, sys

EOC_MIN = 0xFFF8
BAD = 0xFFF7
NAME_RE = re.compile(rb'^[A-Z0-9_]{1,8}( {0,7})[A-Z0-9_ ]{3}$')


class Vol:
    def __init__(self, path, writable=False):
        self.path = path
        self.f = open(path, 'r+b' if writable else 'rb')
        self.data = bytearray(self.f.read())
        d = self.data
        if len(d) < 512 or d[510:512] != b'\x55\xaa':
            raise SystemExit('not a FAT volume: no boot signature')
        (self.bps, self.spc, self.reserved, self.nfats, self.rootent, tot16,
         self.media, self.spf) = struct.unpack_from('<HBHBHHBH', d, 11)
        tot32 = struct.unpack_from('<I', d, 32)[0]
        self.total = tot16 or tot32
        if self.bps != 512 or self.nfats != 2 or self.spc == 0 or self.spc & (self.spc - 1):
            raise SystemExit('unsupported geometry bps=%d nfats=%d spc=%d' % (self.bps, self.nfats, self.spc))
        self.root_sectors = (self.rootent * 32 + 511) // 512
        self.fat1 = self.reserved * 512
        self.fat2 = (self.reserved + self.spf) * 512
        self.root = (self.reserved + 2 * self.spf) * 512
        self.data_start = (self.reserved + 2 * self.spf + self.root_sectors)
        self.clusters = (self.total - self.data_start) // self.spc
        self.cbytes = self.spc * 512
        if not (4085 <= self.clusters <= 65524):
            raise SystemExit('not FAT16: %d clusters' % self.clusters)

    def fat_get(self, copy, c):
        base = self.fat1 if copy == 1 else self.fat2
        return struct.unpack_from('<H', self.data, base + 2 * c)[0]

    def fat_set(self, c, v):
        for base in (self.fat1, self.fat2):
            struct.pack_into('<H', self.data, base + 2 * c, v)

    def cluster_off(self, c):
        return (self.data_start + (c - 2) * self.spc) * 512

    def entries(self):
        """Yield (index, offset, raw32) of root entries up to the end marker."""
        for i in range(self.rootent):
            off = self.root + 32 * i
            e = bytes(self.data[off:off + 32])
            if e[0] == 0:
                return
            yield i, off, e

    def save(self):
        self.f.seek(0)
        self.f.write(self.data)
        self.f.flush()


def parse_name(e):
    base = e[0:8].rstrip(b' ').decode('latin1')
    ext = e[8:11].rstrip(b' ').decode('latin1')
    return base + ('.' + ext if ext else '')


def analyse(v):
    errors, warns, files = [], [], []
    # FAT copies
    f1 = bytes(v.data[v.fat1:v.fat1 + v.spf * 512])
    f2 = bytes(v.data[v.fat2:v.fat2 + v.spf * 512])
    if f1 != f2:
        n = sum(1 for i in range(0, len(f1), 2) if f1[i:i + 2] != f2[i:i + 2])
        errors.append('FAT copies differ in %d entries' % n)
    owner = {}
    seen_names = {}
    for idx, off, e in v.entries():
        if e[0] == 0xE5:
            continue
        attr = e[11]
        if attr == 0x0F or attr & 0x08:
            continue
        name = parse_name(e)
        first = struct.unpack_from('<H', e, 26)[0]
        size = struct.unpack_from('<I', e, 28)[0]
        rec = dict(idx=idx, off=off, name=name, attr=attr, first=first, size=size, chain=[])
        if attr & 0x10:
            files.append(rec)      # directories are listed, not followed (R11L has no subdirectories)
            continue
        if not re.match(rb'^[A-Z0-9_]{1,8}[ ]*[A-Z0-9_ ]{3}$', bytes(e[0:11])) or b' ' in e[0:8].rstrip(b' '):
            errors.append('%s: invalid 8.3 name bytes %r' % (name, bytes(e[0:11])))
        if name in seen_names:
            errors.append('%s: duplicate name (entries %d and %d)' % (name, seen_names[name], idx))
        seen_names[name] = idx
        need = (size + v.cbytes - 1) // v.cbytes
        if size == 0 and first != 0:
            errors.append('%s: size 0 but first cluster %d' % (name, first))
        if size > 0 and first == 0:
            errors.append('%s: size %d but no first cluster' % (name, size))
        c = first
        chain = []
        seen = set()
        while c != 0:
            if c < 2 or c > v.clusters + 1:
                errors.append('%s: cluster %d out of range' % (name, c))
                break
            if c in seen:
                errors.append('%s: cycle at cluster %d' % (name, c))
                break
            seen.add(c)
            chain.append(c)
            if c in owner:
                errors.append('%s: cluster %d shared with %s' % (name, c, owner[c]))
            else:
                owner[c] = name
            nx = v.fat_get(1, c)
            if nx >= EOC_MIN:
                break
            if nx == 0:
                errors.append('%s: chain reaches a free entry after cluster %d' % (name, c))
                break
            if nx == BAD:
                errors.append('%s: chain reaches a bad cluster after cluster %d' % (name, c))
                break
            c = nx
        rec['chain'] = chain
        if size > 0 and first != 0 and chain and len(chain) != need:
            errors.append('%s: chain has %d clusters, size %d needs %d' % (name, len(chain), size, need))
        files.append(rec)
    # orphans
    orphans = []
    for c in range(2, v.clusters + 2):
        x = v.fat_get(1, c)
        if x != 0 and x != BAD and c not in owner:
            orphans.append(c)
    if orphans:
        warns.append('%d orphan cluster(s), first %d' % (len(orphans), orphans[0]))
    bad = sum(1 for c in range(2, v.clusters + 2) if v.fat_get(1, c) == BAD)
    free = sum(1 for c in range(2, v.clusters + 2) if v.fat_get(1, c) == 0)
    return errors, warns, files, orphans, dict(free=free, bad=bad, clusters=v.clusters)


def file_bytes(v, rec):
    out = bytearray()
    for c in rec['chain']:
        o = v.cluster_off(c)
        out += v.data[o:o + v.cbytes]
    return bytes(out[:rec['size']])


def repair(v):
    errors, warns, files, orphans, stats = analyse(v)
    log = []
    # FAT2 := FAT1
    n1 = bytes(v.data[v.fat1:v.fat1 + v.spf * 512])
    if n1 != bytes(v.data[v.fat2:v.fat2 + v.spf * 512]):
        v.data[v.fat2:v.fat2 + v.spf * 512] = n1
        log.append('FAT2 rewritten from FAT1')
    for rec in files:
        if rec['attr'] & 0x10:
            continue
        need = (rec['size'] + v.cbytes - 1) // v.cbytes
        chain = rec['chain']
        if rec['size'] == 0 and rec['first'] != 0:
            struct.pack_into('<H', v.data, rec['off'] + 26, 0)
            for c in chain:
                v.fat_set(c, 0)
            log.append('%s: freed chain of an empty file' % rec['name'])
        elif chain and len(chain) > need:
            for c in chain[need:]:
                v.fat_set(c, 0)
            v.fat_set(chain[need - 1], 0xFFFF)
            log.append('%s: freed %d trailing cluster(s)' % (rec['name'], len(chain) - need))
        elif chain and len(chain) < need:
            newsize = len(chain) * v.cbytes
            struct.pack_into('<I', v.data, rec['off'] + 28, newsize)
            log.append('%s: size %d -> %d (chain capacity)' % (rec['name'], rec['size'], newsize))
    errors, warns, files, orphans, stats = analyse(v)
    for c in orphans:
        v.fat_set(c, 0)
    if orphans:
        log.append('freed %d orphan cluster(s)' % len(orphans))
    v.save()
    return log


def main(argv):
    if len(argv) < 2 or argv[1] in ('-h', '--help'):
        print(__doc__)
        return 2
    path = argv[1]
    opts = argv[2:]
    try:
        v = Vol(path, writable='--repair' in opts)
    except OSError as ex:
        print('cannot open %s: %s' % (path, ex), file=sys.stderr)
        return 2
    if '--repair' in opts:
        for line in repair(v):
            print('repair:', line)
    errors, warns, files, orphans, stats = analyse(v)
    for e in errors:
        print('ERROR:', e)
    for w in warns:
        print('WARN:', w)
    nfiles = sum(1 for r in files if not r['attr'] & 0x10)
    print('%s: %d file(s), %d free / %d clusters, %d error(s), %d warning(s)'
          % (os.path.basename(path), nfiles, stats['free'], stats['clusters'], len(errors), len(warns)))
    if '--manifest' in opts:
        for rec in sorted(files, key=lambda r: r['name']):
            if rec['attr'] & 0x10:
                continue
            data = file_bytes(v, rec)
            print('MANIFEST %-12s %8d %s' % (rec['name'], rec['size'], hashlib.sha256(data).hexdigest()))
    if '--extract' in opts:
        d = opts[opts.index('--extract') + 1]
        os.makedirs(d, exist_ok=True)
        for rec in files:
            if rec['attr'] & 0x10:
                continue
            with open(os.path.join(d, rec['name']), 'wb') as fo:
                fo.write(file_bytes(v, rec))
    if errors or ('--strict' in opts and warns):
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
