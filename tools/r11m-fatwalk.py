#!/usr/bin/env python3
"""R11M independent FAT16 directory-tree walker, checker, manifest writer and extractor.

Written for the R11M subdirectory work; it shares no code with kernel/fat16.c.  It walks the root
and every subdirectory (following each directory's own cluster chain), and checks:

  ERRORS (volume is inconsistent)
    FAT copy mismatch; cluster out of range; cycle; chain not ending in an end marker; chain reaching
    a free/bad entry; a cluster owned by two files/directories; file chain length != ceil(size/cluster);
    size 0 with a first cluster / size > 0 without one; directory with a nonzero size field;
    duplicate names in one directory; invalid 8.3 name bytes; directory without '.' as entry 0 and
    '..' as entry 1; '.' not pointing at the directory itself; '..' not pointing at the parent (0 for
    the root); directory loop (a directory reachable twice); entries after the end marker are not read;
    long-name (VFAT) chains: bad ordinal sequence, bad checksum, orphan (reported as errors only with
    --vfat; otherwise long-name entries are skipped as the R11L/R11M kernel does).
  WARNINGS
    orphan clusters (allocated, reachable from nowhere).

Usage:
  r11m-fatwalk.py IMG                 check; exit 0 consistent (warnings allowed), 1 errors, 2 usage/IO
  r11m-fatwalk.py IMG --strict        warnings count as errors
  r11m-fatwalk.py IMG --tree          print the tree: "<path> <D|F> <size> <sha256 or ->" sorted by path
  r11m-fatwalk.py IMG --extract DIR   write the tree into DIR
  r11m-fatwalk.py IMG --vfat          also decode and validate long names; --tree then prints
                                      "<path> ... " plus "  LFN <long name>" lines for entries that have one
"""
import hashlib, os, re, struct, sys

EOC_MIN = 0xFFF8
BAD = 0xFFF7


class Vol:
    def __init__(self, path):
        self.path = path
        with open(path, 'rb') as f:
            self.data = f.read()
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
        self.data_start = self.reserved + 2 * self.spf + self.root_sectors
        self.clusters = (self.total - self.data_start) // self.spc
        self.cbytes = self.spc * 512
        if not (4085 <= self.clusters <= 65524):
            raise SystemExit('not FAT16: %d clusters' % self.clusters)

    def fat_get(self, c):
        return struct.unpack_from('<H', self.data, self.fat1 + 2 * c)[0]

    def cluster_off(self, c):
        return (self.data_start + (c - 2) * self.spc) * 512


def short_name(e):
    base = e[0:8].rstrip(b' ').decode('latin1')
    ext = e[8:11].rstrip(b' ').decode('latin1')
    return base + ('.' + ext if ext else '')


def lfn_checksum(name11):
    s = 0
    for b in name11:
        s = (((s & 1) << 7) + (s >> 1) + b) & 0xFF
    return s


def chain_of(v, first, errors, what):
    """Cluster chain starting at first; appends errors; returns the list."""
    chain, seen, c = [], set(), first
    while True:
        if c < 2 or c > v.clusters + 1:
            errors.append('%s: cluster %d out of range' % (what, c))
            return chain
        if c in seen:
            errors.append('%s: cycle at cluster %d' % (what, c))
            return chain
        seen.add(c)
        chain.append(c)
        nx = v.fat_get(c)
        if nx >= EOC_MIN:
            return chain
        if nx == 0:
            errors.append('%s: chain reaches a free entry after cluster %d' % (what, c))
            return chain
        if nx == BAD:
            errors.append('%s: chain reaches a bad cluster after cluster %d' % (what, c))
            return chain
        c = nx


def dir_raw(v, first, chain):
    """Raw bytes of a directory: the root region (first == 0) or its cluster chain."""
    if first == 0:
        return v.data[v.root:v.root + v.rootent * 32]
    out = bytearray()
    for c in chain:
        o = v.cluster_off(c)
        out += v.data[o:o + v.cbytes]
    return bytes(out)


def decode_lfn(frags, short11, errors, where):
    """frags: list of 32-byte LFN entries in on-disk order (last logical fragment first)."""
    if not frags:
        return None
    n = len(frags)
    chk = frags[0][13]
    exp = n | 0x40
    if frags[0][0] != exp:
        errors.append('%s: LFN chain starts with ordinal 0x%02x, expected 0x%02x' % (where, frags[0][0], exp))
        return None
    chars = []
    for i, fr in enumerate(frags):
        want = n - i
        if (fr[0] & 0x3F) != want:
            errors.append('%s: LFN ordinal sequence broken (got 0x%02x, want %d)' % (where, fr[0], want))
            return None
        if fr[13] != chk:
            errors.append('%s: LFN checksum differs inside the chain' % where)
            return None
        units = []
        for off, cnt in ((1, 5), (14, 6), (28, 2)):
            for k in range(cnt):
                units.append(struct.unpack_from('<H', fr, off + 2 * k)[0])
        chars.insert(0, units)
    flat = []
    for part in chars:
        flat.extend(part)
    out = []
    for u in flat:
        if u == 0:
            break
        if u == 0xFFFF:
            continue
        out.append(chr(u))
    if lfn_checksum(short11) != chk:
        errors.append('%s: LFN checksum 0x%02x does not match the short name (0x%02x)' % (where, chk, lfn_checksum(short11)))
        return None
    return ''.join(out)


def walk(v, vfat=False):
    """Returns (errors, warnings, nodes) with nodes = list of dict(path, kind, size, first, chain, lfn, attr)."""
    errors, warns, nodes = [], [], []
    owner = {}
    f1 = v.data[v.fat1:v.fat1 + v.spf * 512]
    f2 = v.data[v.fat2:v.fat2 + v.spf * 512]
    if f1 != f2:
        n = sum(1 for i in range(0, len(f1), 2) if f1[i:i + 2] != f2[i:i + 2])
        errors.append('FAT copies differ in %d entries' % n)

    visited_dirs = set()

    def claim(chain, name):
        for c in chain:
            if c in owner:
                errors.append('%s: cluster %d shared with %s' % (name, c, owner[c]))
            else:
                owner[c] = name

    def do_dir(path, first, parent, chain):
        raw = dir_raw(v, first, chain)
        seen = {}
        lfn = []
        for i in range(len(raw) // 32):
            e = raw[32 * i:32 * i + 32]
            if e[0] == 0:
                break
            if e[0] == 0xE5:
                lfn = []
                continue
            attr = e[11]
            if attr == 0x0F:
                lfn.append(e)
                continue
            if attr & 0x08 and not attr & 0x10:
                lfn = []
                continue
            name = short_name(e)
            where = (path.rstrip('/') + '/' + name) if path != '/' else '/' + name
            lname = None
            if lfn:
                if vfat:
                    lname = decode_lfn(lfn, e[0:11], errors, where)
                lfn = []
            first_c = struct.unpack_from('<H', e, 26)[0]
            size = struct.unpack_from('<I', e, 28)[0]
            if attr & 0x10:
                if first != 0 and i < 2:
                    exp = '.' if i == 0 else '..'
                    if name != exp:
                        errors.append('%s: entry %d is %r, expected %r' % (path, i, name, exp))
                    elif i == 0 and first_c != first:
                        errors.append("%s: '.' points to %d, directory is %d" % (path, first_c, first))
                    elif i == 1 and first_c != parent:
                        errors.append("%s: '..' points to %d, parent is %d" % (path, first_c, parent))
                    continue
                if name in ('.', '..'):
                    errors.append('%s: %s entry at index %d' % (path, name, i))
                    continue
            ok83 = rb'^[A-Z0-9_!#$%&\'()@^`{}~-]{1,8} *[A-Z0-9_!#$%&\'()@^`{}~ -]{3}$' if vfat else rb'^[A-Z0-9_]{1,8} *[A-Z0-9_ ]{3}$'
            if not re.match(ok83, bytes(e[0:11])) or b' ' in e[0:8].rstrip(b' '):
                if not (vfat and lname is not None):
                    errors.append('%s: invalid 8.3 name bytes %r' % (where, bytes(e[0:11])))
            key = name.upper()
            if key in seen:
                errors.append('%s: duplicate name (entries %d and %d)' % (where, seen[key], i))
            seen[key] = i
            if attr & 0x10:
                if size != 0:
                    errors.append('%s: directory with size %d' % (where, size))
                if first_c < 2 or first_c > v.clusters + 1:
                    errors.append('%s: directory first cluster %d out of range' % (where, first_c))
                    continue
                if first_c in visited_dirs:
                    errors.append('%s: directory %d reached twice (loop or hard link)' % (where, first_c))
                    continue
                visited_dirs.add(first_c)
                ch = chain_of(v, first_c, errors, where)
                claim(ch, where)
                nodes.append(dict(path=where, kind='D', size=0, first=first_c, chain=ch, lfn=lname, attr=attr))
                do_dir(where, first_c, first, ch)
            else:
                need = (size + v.cbytes - 1) // v.cbytes
                if size == 0 and first_c != 0:
                    errors.append('%s: size 0 but first cluster %d' % (where, first_c))
                if size > 0 and first_c == 0:
                    errors.append('%s: size %d but no first cluster' % (where, size))
                ch = []
                if first_c != 0:
                    ch = chain_of(v, first_c, errors, where)
                    claim(ch, where)
                    if size > 0 and ch and len(ch) != need:
                        errors.append('%s: chain has %d clusters, size %d needs %d' % (where, len(ch), size, need))
                nodes.append(dict(path=where, kind='F', size=size, first=first_c, chain=ch, lfn=lname, attr=attr))

    do_dir('/', 0, 0, [])
    for c in range(2, v.clusters + 2):
        val = v.fat_get(c)
        if val != 0 and val != BAD and c not in owner:
            warns.append('orphan cluster %d' % c)
    return errors, warns, nodes


def read_file(v, node):
    out = bytearray()
    for c in node['chain']:
        o = v.cluster_off(c)
        out += v.data[o:o + v.cbytes]
    return bytes(out[:node['size']])


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    img = argv[1]
    opts = argv[2:]
    strict = '--strict' in opts
    vfat = '--vfat' in opts
    try:
        v = Vol(img)
    except OSError as ex:
        print('cannot read %s: %s' % (img, ex))
        return 2
    errors, warns, nodes = walk(v, vfat)
    if '--tree' in opts:
        for n in sorted(nodes, key=lambda x: x['path']):
            h = '-' if n['kind'] == 'D' else hashlib.sha256(read_file(v, n)).hexdigest()
            print('%s %s %d %s' % (n['path'], n['kind'], n['size'], h))
            if vfat and n['lfn']:
                print('  LFN %s' % n['lfn'])
    if '--extract' in opts:
        dest = opts[opts.index('--extract') + 1]
        for n in nodes:
            p = os.path.join(dest, n['path'].lstrip('/'))
            if n['kind'] == 'D':
                os.makedirs(p, exist_ok=True)
            else:
                os.makedirs(os.path.dirname(p), exist_ok=True)
                with open(p, 'wb') as f:
                    f.write(read_file(v, n))
    for e in errors:
        print('ERROR: ' + e)
    for w in warns:
        print('WARN: ' + w)
    nf = sum(1 for n in nodes if n['kind'] == 'F')
    nd = sum(1 for n in nodes if n['kind'] == 'D')
    print('%s: %d files, %d directories, %d errors, %d warnings' % (os.path.basename(img), nf, nd, len(errors), len(warns)))
    if errors or (strict and warns):
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
