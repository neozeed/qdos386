#!/usr/bin/env python3
"""R13A: broken QFNT fixtures for the loader tests, made from a valid font.  Each breaks exactly one rule.
  usage: mkbad.py GOOD.FNT OUTDIR      writes BADMAGIC BADHCRC BADPCRC TRUNC BIG DUPMAP WIDEBITS BADCELL EMPTY BADMAP .FNT"""
import os, struct, sys, zlib

good = bytearray(open(sys.argv[1], "rb").read())
out = sys.argv[2]
os.makedirs(out, exist_ok=True)


def fix(b):
    """recompute both checksums (so that only the intended rule is broken)"""
    b = bytearray(b)
    b[36:40] = struct.pack("<I", zlib.crc32(bytes(b[64:])) & 0xFFFFFFFF)
    b[40:44] = struct.pack("<I", zlib.crc32(bytes(b[:40])) & 0xFFFFFFFF)
    return b


bad = {}
b = bytearray(good); b[0:4] = b"QFNX"; bad["BADMAGIC"] = fix(b)
b = bytearray(good); b[40] ^= 1; bad["BADHCRC"] = b                                 # header checksum wrong
b = bytearray(good); b[700] ^= 0x80; b[40:44] = struct.pack("<I", zlib.crc32(bytes(b[:40])) & 0xFFFFFFFF); bad["BADPCRC"] = b
bad["TRUNC"] = bytearray(good[:len(good) - 7])                                       # truncated
b = bytearray(good) + bytearray(40000); bad["BIG"] = b                               # larger than the maximum
b = bytearray(good); m = list(struct.unpack("<256H", bytes(b[64:576])))
m[0x41] = m[0x42]; b[64:576] = struct.pack("<256H", *m); bad["DUPMAP"] = fix(b)        # 'A' and 'B' share a glyph
b = bytearray(good); gw = b[8]; rb = b[15]
if rb * 8 > gw:
    b[576 + 5 * rb + rb - 1] |= 1                                                     # a pixel right of the glyph width
bad["WIDEBITS"] = fix(b)
b = bytearray(good); b[10] = b[8] - 1 if b[8] > 1 else 0; bad["BADCELL"] = fix(b)       # cell narrower than the glyph
bad["EMPTY"] = bytearray()
b = bytearray(good); m = list(struct.unpack("<256H", bytes(b[64:576])))
m[0x41] = 600; b[64:576] = struct.pack("<256H", *m); bad["BADMAP"] = fix(b)            # a code mapped past the glyphs
for k, v in bad.items():
    open(os.path.join(out, k + ".FNT"), "wb").write(bytes(v))
print("wrote %d broken fonts to %s" % (len(bad), out))
