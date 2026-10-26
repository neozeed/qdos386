#!/usr/bin/env python3
"""Extract text+data of a linked a.out OMAGIC image at a fixed origin.

Writes the raw bytes that belong at the origin (no bss).  Checks that the
image has no relocations and that data follows text with no gap.
"""
import pathlib, struct, sys
src, dst, origin = sys.argv[1], sys.argv[2], int(sys.argv[3], 0)
b = pathlib.Path(src).read_bytes()
magic, text, data, bss, syms, entry, tr, dr = struct.unpack_from('<8I', b)
if magic != 0o407:
    raise SystemExit('not OMAGIC')
if tr or dr:
    raise SystemExit('relocations remain: link is not final')
if entry != origin:
    raise SystemExit('entry %x != origin %x' % (entry, origin))
if len(b) < 32 + text + data:
    raise SystemExit('truncated image')
pathlib.Path(dst).write_bytes(b[32:32 + text + data])
print('origin=%x text=%d data=%d bss=%d entry=%x' % (origin, text, data, bss, entry))
