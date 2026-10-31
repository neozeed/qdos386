#!/usr/bin/env python3
"""R14: configure a built QLTerm program the way MenuConfig configures the QL executable: find the "<<QCFX>>02" block
in the binary and change the named fields in place (offsets from the source's config.h, relative to the block).
  usage: r14-qlterm-config.py IN OUT [FIELD=VALUE ...]
  byte fields: PORT (0 SER1I 1 SER1H 2 SER2I 3 SER2H 4 custom 5 TCP 6 Telnet) BAUD (index: 0 NONE 1 300 .. 4 2400 ..
               10 115200 11 12H75) ECHO TERM COLOUR ENTER DEL CONVLF BELL COOKED OPENMODE TRA PROTO PAPER INK BORDER
  word fields: X Y COLS ROWS RXBUF DLBUF
  strings:     DLDIR CUSTOM INIT PREFIX POSTFIX HANGUP   (QDOS strings: max, length, text; the max is kept)
Only the block is changed; everything else in the program is byte-identical (checked)."""
import re, sys
src = open(sys.argv[1], "rb").read()
h = open(__import__("os").path.join(__import__("os").path.dirname(__file__), "..", "third_party", "qlterm-c-r1", "orig",
                                    "src", "config.h")).read()
OFF = {m.group(1): int(m.group(2)) for m in re.finditer(r"#define CFG_(\w+) (\d+)", h)}
STR = {"DLDIR", "CUSTOM", "INIT", "PREFIX", "POSTFIX", "HANGUP"}
WORD = {"X", "Y", "COLS", "ROWS", "RXBUF", "DLBUF"}
b = bytearray(src)
# the block is config.c's config_blob (it holds more than one "<<QCFX>>02" header): locate it by its whole content
cfg = bytes(int(x) for x in re.findall(r"\d+", open(__import__("os").path.join(__import__("os").path.dirname(__file__), "..",
            "third_party", "qlterm-c-r1", "orig", "src", "config.c")).read().split("{", 1)[1].split("}")[0]))
base = b.find(cfg)
assert base >= 0 and b.find(cfg, base + 1) < 0, "exactly one configuration block"
for a in sys.argv[3:]:
    k, v = a.split("=", 1)
    o = base + OFF[k]
    if k in STR:
        mx = (b[o] << 8) | b[o + 1]
        t = v.encode("latin-1")
        assert len(t) <= mx, "%s longer than %d" % (k, mx)
        b[o + 2:o + 4] = bytes([len(t) >> 8, len(t) & 255])
        b[o + 4:o + 4 + mx] = t + bytes(mx - len(t))
    elif k in WORD:
        n = int(v, 0); b[o:o + 2] = bytes([n >> 8 & 255, n & 255])
    else:
        n = int(v, 0); b[o] = n & 255
diff = [i for i in range(len(b)) if b[i] != src[i]]
assert all(base <= i < base + len(cfg) for i in diff), "changes outside the block"
open(sys.argv[2], "wb").write(b)
print("configured %s: %s (%d bytes changed in the block at 0x%x)" % (sys.argv[2], " ".join(sys.argv[3:]), len(diff), base))
