#!/usr/bin/env python3
"""R13A: the QFNT v1 font format (docs/R13A-FONT-FORMAT.md) - writer, strict reader/validator, and converters.

  qfnt.py pcf SRC.pcf[.gz] OUT.FNT NAME ENCODING FIRST LAST CELLW CELLH
          an X11 PCF bitmap font (the public-domain / unrestricted xfonts-cyrillic KOI8 fonts) -> QFNT, codes
          FIRST..LAST only (0x20..0x7E: the ASCII part, identical in KOI8-R), glyphs placed on the font's baseline
  qfnt.py minerva TB_F0.s TB_F1.s OUT.FNT [ROM]
          Minerva 1.98's two QL fonts from the GAS source -> one QFNT with the QL mapping (all 256 codes; $00..$1E and
          codes in neither font show tb_f0's first glyph, as Minerva's cs_char); with ROM: rows cross-checked
          byte for byte against min198.rom
  qfnt.py check FILE.FNT          validate (the same rules as the kernel loader) and print the header
  qfnt.py c FILE.FNT SYMBOL OUT.c write a kernel built-in table (struct kfont_builtin, kernel/fontreg.h)
  qfnt.py png FILE.FNT OUT.png    a 16 x 16 specimen of the mapped codes
"""
import gzip, hashlib, re, struct, sys, zlib

MAGIC = b"QFNT"
VERSION = 1
HDR = 64
ENC = {"ascii": 1, "ql": 2, "cp437": 3, "koi8r": 4}
MAX_FILE = 64 + 512 + 512 * 32 * 2           # header + map + 512 glyphs of 16 x 32
NONE = 0xFFFF


class Font:
    def __init__(self, name, enc, gw, gh, cw, ch, xo, yo, glyphs, cmap, default):
        self.name, self.enc, self.gw, self.gh, self.cw, self.ch, self.xo, self.yo = name, enc, gw, gh, cw, ch, xo, yo
        self.glyphs, self.map, self.default = glyphs, cmap, default     # glyphs: list of gh row ints (bit gw-1 = left)

    def rb(self):
        return (self.gw + 7) // 8

    def row_bytes(self, r):
        """a row (gw bits, leftmost = most significant) as rb() bytes, MSB first, padded on the right"""
        v = r << (8 * self.rb() - self.gw)
        return v.to_bytes(self.rb(), "big")

    def encode(self):
        body = b"".join(struct.pack("<H", m) for m in self.map)
        for g in self.glyphs:
            body += b"".join(self.row_bytes(r) for r in g)
        name = self.name.encode("ascii")[:19].ljust(20, b"\0")
        h = struct.pack("<4sHHBBBBBBBBHHHHIIII", MAGIC, VERSION, HDR, self.gw, self.gh, self.cw, self.ch, self.xo, self.yo,
                        0, self.rb(), len(self.glyphs), self.default, 256, self.enc, HDR, HDR + 512, len(body),
                        zlib.crc32(body) & 0xFFFFFFFF)
        h += struct.pack("<I", zlib.crc32(h) & 0xFFFFFFFF) + name
        assert len(h) == HDR
        return h + body


def parse(data):
    """the strict reader: returns a Font or raises ValueError with the rule that failed"""
    if len(data) < HDR:
        raise ValueError("truncated header")
    if len(data) > MAX_FILE:
        raise ValueError("file larger than %d bytes" % MAX_FILE)
    (mg, ver, hl, gw, gh, cw, ch, xo, yo, bo, rb, cnt, dflt, nmap, enc, moff, boff, plen, pcrc) = \
        struct.unpack("<4sHHBBBBBBBBHHHHIIII", data[:40])
    hcrc = struct.unpack("<I", data[40:44])[0]
    if mg != MAGIC:
        raise ValueError("bad magic")
    if ver != VERSION:
        raise ValueError("unsupported version %d" % ver)
    if hl != HDR:
        raise ValueError("bad header length")
    if zlib.crc32(data[:40]) & 0xFFFFFFFF != hcrc:
        raise ValueError("header checksum")
    if not (1 <= gw <= 16 and 1 <= gh <= 32 and gw <= cw <= 32 and gh <= ch <= 32 and xo + gw <= cw and yo + gh <= ch):
        raise ValueError("bad glyph or cell size")
    if bo != 0 or rb != (gw + 7) // 8:
        raise ValueError("bad bit order or row bytes")
    if not (1 <= cnt <= 512) or dflt >= cnt or nmap != 256 or enc not in ENC.values():
        raise ValueError("bad glyph count, default, map size or encoding")
    if moff != HDR or boff != HDR + 512 or plen != len(data) - HDR or plen != 512 + cnt * gh * rb:
        raise ValueError("bad offsets or payload length")
    if zlib.crc32(data[HDR:]) & 0xFFFFFFFF != pcrc:
        raise ValueError("payload checksum")
    cmap = list(struct.unpack("<256H", data[moff:moff + 512]))
    used = set()
    for m in cmap:
        if m == NONE:
            continue
        if m >= cnt:
            raise ValueError("map entry beyond the glyphs")
        if m in used:
            raise ValueError("duplicate mapping")
        used.add(m)
    glyphs = []
    pad = 8 * rb - gw
    for i in range(cnt):
        g = []
        for r in range(gh):
            o = boff + (i * gh + r) * rb
            v = int.from_bytes(data[o:o + rb], "big")
            if v & ((1 << pad) - 1):
                raise ValueError("glyph %d: bits outside the glyph width" % i)
            g.append(v >> pad)
        glyphs.append(g)
    name = data[44:64].split(b"\0")[0].decode("ascii", "replace")
    return Font(name, enc, gw, gh, cw, ch, xo, yo, glyphs, cmap, dflt)


# ---- PCF (X11 compiled bitmap font) ----------------------------------------------------------------------------
def read_pcf(path):
    d = gzip.open(path).read() if path.endswith(".gz") else open(path, "rb").read()
    assert d[:4] == b"\x01fcp", "not a PCF file"
    n = struct.unpack("<i", d[4:8])[0]
    toc = {}
    for i in range(n):
        t, f, s, o = struct.unpack("<iiii", d[8 + 16 * i:24 + 16 * i])
        toc[t] = (f, s, o)

    def table(t):
        f, s, o = toc[t]
        fmt = struct.unpack("<i", d[o:o + 4])[0]
        be = ">" if fmt & 4 else "<"
        return fmt, be, o + 4

    fmt, be, o = table(1 << 2)                                   # PCF_METRICS
    if fmt & 0x100:
        cnt = struct.unpack(be + "h", d[o:o + 2])[0]
        o += 2
        met = [tuple(b - 0x80 for b in d[o + 5 * i:o + 5 * i + 5]) for i in range(cnt)]
    else:
        cnt = struct.unpack(be + "i", d[o:o + 4])[0]
        o += 4
        met = [struct.unpack(be + "hhhhhH", d[o + 12 * i:o + 12 * i + 12])[:5] for i in range(cnt)]
    fmt, be, o = table(1 << 3)                                   # PCF_BITMAPS
    gcount = struct.unpack(be + "i", d[o:o + 4])[0]
    offs = struct.unpack(be + "%di" % gcount, d[o + 4:o + 4 + 4 * gcount])
    data0 = o + 4 + 4 * gcount + 16
    pad = 1 << (fmt & 3)
    msbit = bool(fmt & 8)
    msbyte = bool(fmt & 4)
    unit = 1 << ((fmt >> 4) & 3)
    fmt, be, o = table(1 << 5)                                   # PCF_BDF_ENCODINGS
    mn2, mx2, mn1, mx1, dflt = struct.unpack(be + "hhhhh", d[o:o + 10])
    nidx = (mx2 - mn2 + 1) * (mx1 - mn1 + 1)
    idx = struct.unpack(be + "%dH" % nidx, d[o + 10:o + 10 + 2 * nidx])
    acc = toc.get(1 << 8) or toc.get(1 << 1)                     # BDF_ACCELERATORS / ACCELERATORS
    af = struct.unpack("<i", d[acc[2]:acc[2] + 4])[0]
    abe = ">" if af & 4 else "<"
    fasc, fdesc = struct.unpack(abe + "ii", d[acc[2] + 12:acc[2] + 20])

    def glyph(gi):
        lsb, rsb, width, asc, desc = met[gi]
        w, h = rsb - lsb, asc + desc
        stride = ((w + 7) // 8 + pad - 1) // pad * pad
        rows = []
        for r in range(h):
            raw = bytearray(d[data0 + offs[gi] + r * stride:data0 + offs[gi] + (r + 1) * stride])
            if msbyte != msbit and unit > 1:                     # swap bytes within scan units
                for k in range(0, len(raw), unit):
                    raw[k:k + unit] = raw[k:k + unit][::-1]
            if not msbit:
                raw = bytearray(int("{:08b}".format(b)[::-1], 2) for b in raw)
            v = int.from_bytes(bytes(raw), "big") >> (8 * stride - w) if w > 0 else 0
            rows.append(v)
        return dict(lsb=lsb, w=w, width=width, asc=asc, desc=desc, rows=rows)

    def code(c):
        if mn1 == 0 and mx1 == 0:
            if mn2 <= c <= mx2:
                gi = idx[c - mn2]
                return None if gi == 0xFFFF else gi
        return None
    return dict(glyph=glyph, code=code, ascent=fasc, descent=fdesc, raw=d)


def from_pcf(path, name, enc, first, last, cw, ch):
    p = read_pcf(path)
    gw, gh = cw, p["ascent"] + p["descent"]
    assert gh <= ch, "font taller than the cell"
    glyphs, cmap = [], [NONE] * 256
    box = None
    for c in range(first, last + 1):
        gi = p["code"](c)
        assert gi is not None, "code %02X missing" % c
        g = p["glyph"](gi)
        assert g["width"] == cw, "not a %d-wide fixed font at %02X" % (cw, c)
        rows = [0] * gh
        for r, v in enumerate(g["rows"]):
            y = p["ascent"] - g["asc"] + r
            if 0 <= y < gh:
                x = g["lsb"]
                rows[y] = (v << (gw - x - g["w"])) & ((1 << gw) - 1) if gw - x - g["w"] >= 0 else v >> (x + g["w"] - gw)
        cmap[c] = len(glyphs)
        glyphs.append(rows)
    # the default (unmapped) glyph: an outlined box, drawn here (not from the source font)
    box = [0] * gh
    col = lambda c: 1 << (gw - 1 - c)
    for r in range(1, gh - 1):
        if r in (1, gh - 2):
            for c in range(1, gw - 1):
                box[r] |= col(c)
        else:
            box[r] = col(1) | col(gw - 2)
    glyphs.append(box)
    return Font(name, enc, gw, gh, cw, ch, 0, 0, glyphs, cmap, len(glyphs) - 1), hashlib.sha256(p["raw"]).hexdigest()


# ---- Minerva 1.98 (QL) -------------------------------------------------------------------------------------------
def minerva_rows(path):
    first, rows = None, []
    for line in open(path, encoding="latin-1"):
        m = re.match(r"^/\* QL:\s+dc\.b\s+(\S+)", line)
        if not m:
            continue
        t = m.group(1)
        if first is None:
            first = int(re.match(r"^\$([0-9a-fA-F]+),", t).group(1), 16)
            continue
        assert re.match(r"^[O.]{5}$", t), "%s: unexpected row %r" % (path, t)
        rows.append(sum(0x40 >> i for i, ch in enumerate(t) if ch == "O"))
    assert first is not None and len(rows) % 9 == 0
    return first, rows


def from_minerva(f0p, f1p, rom=None):
    (f0, r0), (f1, r1) = minerva_rows(f0p), minerva_rows(f1p)
    if rom is not None:
        assert hashlib.sha256(rom).hexdigest() == "aafbd3b0b2bf60d2cdeb139a355d552908b041ab021a1ff1b0be5547d9bcf2a2"
        for first, rows in ((f0, r0), (f1, r1)):
            blob = bytes([first, len(rows) // 9 - 1] + rows)
            assert rom.count(blob) == 1, "source and ROM differ"
    n0, n1 = len(r0) // 9, len(r1) // 9
    # glyphs: tb_f0 $1F..$7F then tb_f1 $80..$FF (tb_f1's codes above $FF cannot be reached from a byte: omitted)
    glyphs, cmap = [], [NONE] * 256
    for i in range(n0):
        glyphs.append([(b >> 2) & 0x1F for b in r0[9 * i:9 * i + 9]])          # bits 6..2 -> 5-bit rows
    for i in range(min(n1, 0x100 - f1)):
        glyphs.append([(b >> 2) & 0x1F for b in r1[9 * i:9 * i + 9]])
    for c in range(256):
        if f0 < c < f0 + n0:                                    # $1F itself is the default glyph (0)
            cmap[c] = c - f0
        elif f1 <= c < 0x100:
            cmap[c] = n0 + c - f1
    # QL cell: 6 x 10; the 5 pixels are bits 6..2 of the ROM byte, i.e. columns 1..5 of the cell (bit 7 = column 0 blank)
    return Font("Minerva 1.98 QL", ENC["ql"], 5, 9, 6, 10, 1, 0, glyphs, cmap, 0)


def to_c(fnt_bytes, sym, src_note):
    f = parse(fnt_bytes)
    out = ["/* Generated by tools/r13a-font/qfnt.py from %s - do not edit.  QFNT v1 image (docs/R13A-FONT-FORMAT.md),"
           % src_note, " * validated by the kernel's loader rules at build time.  SHA-256 %s */"
           % hashlib.sha256(fnt_bytes).hexdigest(), '#include "fontreg.h"', "",
           "const unsigned char %s[%d] = {" % (sym, len(fnt_bytes))]
    for i in range(0, len(fnt_bytes), 16):
        out.append("\t" + ", ".join("0x%02x" % b for b in fnt_bytes[i:i + 16]) + ",")
    out.append("};")
    out.append("const unsigned int %s_len = %d;   /* %s: %dx%d glyphs in %dx%d cells, %d glyphs */"
               % (sym, len(fnt_bytes), f.name, f.gw, f.gh, f.cw, f.ch, len(f.glyphs)))
    return "\n".join(out) + "\n"


def specimen(f, path):
    from PIL import Image
    S = 3
    img = Image.new("RGB", (16 * f.cw * S, 16 * f.ch * S), (0, 0, 96))
    px = img.load()
    for c in range(256):
        gi = f.map[c] if f.map[c] != NONE else f.default
        ox, oy = (c % 16) * f.cw * S, (c // 16) * f.ch * S
        for y, row in enumerate(f.glyphs[gi]):
            for x in range(f.gw):
                if row & (1 << (f.gw - 1 - x)):
                    for a in range(S):
                        for b in range(S):
                            px[ox + (f.xo + x) * S + a, oy + (f.yo + y) * S + b] = (255, 255, 200)
    img.save(path)


if __name__ == "__main__":
    a = sys.argv[1:]
    if a[0] == "pcf":
        f, h = from_pcf(a[1], a[3], ENC[a[4]], int(a[5], 0), int(a[6], 0), int(a[7]), int(a[8]))
        b = f.encode()
        parse(b)
        open(a[2], "wb").write(b)
        print("%s: %s from %s (sha256 %s), %d glyphs %dx%d in %dx%d, %d bytes, sha256 %s"
              % (a[2], f.name, a[1], h, len(f.glyphs), f.gw, f.gh, f.cw, f.ch, len(b), hashlib.sha256(b).hexdigest()))
    elif a[0] == "minerva":
        rom = open(a[4], "rb").read() if len(a) > 4 else None
        f = from_minerva(a[1], a[2], rom)
        b = f.encode()
        parse(b)
        open(a[3], "wb").write(b)
        print("%s: %s, %d glyphs, %d bytes, sha256 %s%s" % (a[3], f.name, len(f.glyphs), len(b),
              hashlib.sha256(b).hexdigest(), ", rows checked against the ROM" if rom else ""))
    elif a[0] == "check":
        try:
            f = parse(open(a[1], "rb").read())
            print("%s: OK %r enc %d glyph %dx%d cell %dx%d at (%d,%d) glyphs %d default %d"
                  % (a[1], f.name, f.enc, f.gw, f.gh, f.cw, f.ch, f.xo, f.yo, len(f.glyphs), f.default))
        except ValueError as e:
            print("%s: REJECTED %s" % (a[1], e))
            sys.exit(1)
    elif a[0] == "c":
        open(a[3], "w").write(to_c(open(a[1], "rb").read(), a[2], a[1]))
    elif a[0] == "png":
        specimen(parse(open(a[1], "rb").read()), a[2])
