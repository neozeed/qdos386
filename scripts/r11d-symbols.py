#!/usr/bin/env python3
"""List the external symbols of an i386 a.out static library (R11D audit).

Host nm cannot read GAS 1.38 a.out objects, so this reads the archive members
directly.  Output: one line per external symbol: "<kind> <name> <member>", where
kind is T (text), D (data), B (bss), A (absolute) or U (undefined reference).
    usage: python3 scripts/r11d-symbols.py LIB.a [LIB.a ...]
"""
import struct, sys

def members(data):
    assert data[:8] == b"!<arch>\n", "not an ar archive"
    i = 8
    while i + 60 <= len(data):
        hdr = data[i:i + 60]
        name = hdr[:16].decode("latin-1").strip()
        size = int(hdr[48:58].decode("latin-1").strip())
        body = data[i + 60:i + 60 + size]
        yield name.rstrip("/"), body
        i += 60 + size + (size & 1)

def symbols(obj):
    if len(obj) < 32:
        return
    magic = struct.unpack("<I", obj[:4])[0] & 0xffff
    if magic not in (0o407, 0o410, 0o413, 0o314, 0o315):
        return
    text, data, bss, syms = struct.unpack("<IIII", obj[4:20])
    # a.out layout: header, text, data, text reloc, data reloc, symbols, strings
    trsize, drsize = struct.unpack("<II", obj[24:32])
    symoff = 32 + text + data + trsize + drsize
    strtab = symoff + syms
    for k in range(0, syms, 12):
        strx, typ, _other, _desc, value = struct.unpack("<IBBHI", obj[symoff + k:symoff + k + 12])
        if strx == 0:
            continue
        s = strtab + strx
        name = obj[s:obj.index(b"\0", s)].decode("latin-1")
        ext = typ & 0x01
        base = typ & 0x1e
        if not ext:
            continue
        if base == 0x00:
            kind = "U" if value == 0 else "C"
        else:
            kind = {0x02: "A", 0x04: "T", 0x06: "D", 0x08: "B"}.get(base, "?")
        yield kind, name

def main(paths):
    for p in paths:
        with open(p, "rb") as f:
            data = f.read()
        for mname, body in members(data):
            for kind, name in symbols(body):
                print(kind, name, mname)

if __name__ == "__main__":
    main(sys.argv[1:])
