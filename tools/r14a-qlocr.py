#!/usr/bin/env python3
"""R14A: read the text of a QLBox screen dump (512x256 PPM, QL mode 4) as 25 rows x 84 columns.

The glyphs are taken from the QL ROM's own character font (the JS ROM QLBox runs: font at 0xADB0, first code 0x1F,
96 glyphs of 9 bytes) and matched cell by cell, so the result is exact or marked: a cell that matches no glyph is
shown as '¿'.  Calibrated on a known line of Hack's QL screen: the CON window (Hack's _condetails: 512x256,
border width 1) puts the text at x = 2, y = 1, cells of 6 x 10 pixels, glyph rows from the cell's second row.
Ink is any pixel with green > 100 (Hack's QL text is green; the border is red); a cell matching an inverted glyph
(reverse video) is reported as that character and listed after the screen.
    usage: python3 tools/r14a-qlocr.py ROM.bin SCREEN.ppm  ->  "NN|text" rows (trailing spaces removed)
"""
import sys

def main(romp, ppm):
    rom = open(romp, "rb").read()
    F = 0xADB0
    first, cnt = rom[F], rom[F + 1]
    glyphs = {}
    for i in range(cnt):
        g = rom[F + 2 + 9 * i:F + 11 + 9 * i]
        rows = []
        for y in range(10):
            b = g[y - 1] if 1 <= y <= 9 else 0
            rows.append(tuple(bool(b & (0x80 >> x)) for x in range(6)))
        ch = chr(first + i)
        if ch.isprintable() and first + i < 0x7F:
            glyphs.setdefault(tuple(rows), ch)
    inv = {tuple(tuple(not p for p in r) for r in k): v for k, v in glyphs.items()}
    d = open(ppm, "rb").read()
    assert d.startswith(b"P6\n512 256\n255\n"), "not a QLBox 512x256 dump"
    px = d[15:]
    def ink(x, y):
        return px[3 * (y * 512 + x) + 1] > 100
    rev = []
    for r in range(25):
        line = []
        for c in range(84):
            cell = tuple(tuple(ink(2 + 6 * c + x, 1 + 10 * r + y) for x in range(6)) for y in range(10))
            ch = glyphs.get(cell)
            if ch is None and cell in inv:
                ch = inv[cell]; rev.append((r, c))
            line.append(ch if ch is not None else "¿")
        print("%02d|%s" % (r, "".join(line).rstrip()))
    if rev:
        print("REVERSE " + " ".join("%d,%d" % rc for rc in rev))

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
