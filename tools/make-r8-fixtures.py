#!/usr/bin/env python3
"""Negative R8 a.out fixtures, derived from a good image (LARGE.AOU).

Usage: make-r8-fixtures.py good.aout outdir

  OVER.AOU   header BSS raised so text+data+BSS exceeds 224 KiB  -> LAYOUT -24
  TRUNC.AOU  last 4096 bytes removed                              -> SIZE -20
  LONG.AOU   16 extra bytes appended                              -> SIZE -20
  ENTRY.AOU  entry point moved outside the text                   -> ENTRY -25
  MAGIC.AOU  magic number 0407 replaced by 0413                   -> MAGIC -21
Each is a read-only fixture: the good image is never changed.
"""
import pathlib, struct, sys

def main():
    src = pathlib.Path(sys.argv[1]).read_bytes()
    out = pathlib.Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    magic, text, data, bss, syms, entry = struct.unpack('<6I', src[:24])
    assert magic == 0o407, 'source is not OMAGIC'

    def hdr(m=magic, t=text, d=data, b=bss, s=syms, e=entry):
        return struct.pack('<6I', m, t, d, b, s, e) + src[24:32]

    body = src[32:]
    (out / 'OVER.AOU').write_bytes(hdr(b=0x40000) + body)
    (out / 'TRUNC.AOU').write_bytes(hdr() + body[:-4096])
    (out / 'LONG.AOU').write_bytes(hdr() + body + b'\0' * 16)
    (out / 'ENTRY.AOU').write_bytes(hdr(e=text + data + 0x10000) + body)
    (out / 'MAGIC.AOU').write_bytes(struct.pack('<I', 0o413) + src[4:32] + body)

if __name__ == '__main__':
    main()
