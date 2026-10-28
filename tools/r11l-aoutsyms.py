#!/usr/bin/env python3
"""R11L: print the symbol table of an i386 a.out executable (sorted by address) or look up addresses.
usage: r11l-aoutsyms.py FILE [ADDR ...]   (no ADDR: dump all text/data/bss symbols)"""
import struct, sys, bisect


def load(path):
    d = open(path, 'rb').read()
    magic, text, data, bss, syms, entry, trsize, drsize = struct.unpack('<8I', d[:32])
    off = 32 + text + data + trsize + drsize
    stroff = off + syms
    out = []
    for i in range(syms // 12):
        strx, typ, other, desc, val = struct.unpack('<IBBhI', d[off + 12 * i:off + 12 * i + 12])
        if typ & 0xe0 or not strx:
            continue
        name = d[stroff + strx:d.index(b'\0', stroff + strx)].decode('latin1')
        if (typ & 0x1e) in (4, 6, 8, 2):    # text, data, bss, abs
            out.append((val, name, typ))
    out.sort()
    return out


def main():
    s = load(sys.argv[1])
    if len(sys.argv) == 2:
        for v, n, t in s:
            print('%08x %s' % (v, n))
        return
    keys = [v for v, n, t in s]
    for a in sys.argv[2:]:
        x = int(a, 16)
        i = bisect.bisect_right(keys, x) - 1
        print('%s -> %s+%#x' % (a, s[i][1], x - s[i][0]) if i >= 0 else a)


main()
