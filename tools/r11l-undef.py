#!/usr/bin/env python3
"""R11L: list the symbols that a set of a.out i386 objects and archives leaves undefined, and for each
the objects that reference it.  Resolution follows the linker: objects given on the command line are
always included, archive members only if they define a symbol that is currently undefined (repeated
until nothing changes), in command-line order of archives.
usage: r11l-undef.py OBJ.o ... LIB.a ...
"""
import struct, sys


def syms_of(d):
    magic, text, data, bss, syms, entry, trsize, drsize = struct.unpack('<8I', d[:32])
    off = 32 + text + data + trsize + drsize
    stroff = off + syms
    defs, undefs = set(), set()
    for i in range(syms // 12):
        strx, typ, other, desc, val = struct.unpack('<IBBhI', d[off + 12 * i:off + 12 * i + 12])
        if typ & 0xe0:      # stab
            continue
        if not strx:
            continue
        end = d.index(b'\0', stroff + strx)
        name = d[stroff + strx:end].decode('latin1')
        t = typ & 0x1f
        if t == 0x01 and val == 0:       # N_EXT undefined
            undefs.add(name)
        elif t == 0x01 and val != 0:     # common
            defs.add(name)
        elif typ & 1:
            defs.add(name)
    return defs, undefs


def members(path):
    d = open(path, 'rb').read()
    if not d.startswith(b'!<arch>\n'):
        yield path, d
        return
    p = 8
    while p + 60 <= len(d):
        name = d[p:p + 16].decode('latin1').strip()
        size = int(d[p + 48:p + 58].decode().strip())
        body = d[p + 60:p + 60 + size]
        if body[:4] in (b'\x07\x01\x00\x00', b'\x07\x01\x00\x00'):
            yield path + '(' + name + ')', body
        p += 60 + size + (size & 1)


def main():
    objs, libs = [], []
    for a in sys.argv[1:]:
        (libs if a.endswith('.a') else objs).append(a)
    defined, undef, refs = set(), set(), {}
    for o in objs:
        for n, body in members(o):
            d, u = syms_of(body)
            defined |= d
            for s in u:
                refs.setdefault(s, []).append(n.split('/')[-1])
            undef |= u
    undef -= defined
    pools = []
    for l in libs:
        pools.append([(n,) + syms_of(b) for n, b in members(l)])
    changed = True
    used = set()
    while changed:
        changed = False
        for pool in pools:
            for (n, d, u) in pool:
                if n in used:
                    continue
                if d & undef:
                    used.add(n)
                    defined |= d
                    for s in u:
                        refs.setdefault(s, []).append(n.split('/')[-1])
                    undef |= u
                    undef -= defined
                    changed = True
    for s in sorted(undef):
        print('%-24s %s' % (s, ' '.join(sorted(set(refs.get(s, [])))[:4])))
    print('# %d undefined' % len(undef), file=sys.stderr)


main()
