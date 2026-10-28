#!/usr/bin/env python3
"""R11K TEST ONLY: emit an assembly file holding BYTES bytes of deterministic .data (a multiplicative
sequence), so a PC kernel can be made as large as wanted with data it can verify at run time.
platform/pc/bigk_check.c recomputes the same sequence and compares every dword; any sector the loader
dropped, duplicated or placed wrongly shows up as the first mismatching offset.
  word[i] = (i * 2654435761 + 0x1234567) mod 2^32        (i = 0 .. BYTES/4 - 1)
usage: r11k-gen-bigk.py BYTES out.s
"""
import sys
n = int(sys.argv[1], 0)
if n % 4 or n < 0:
    raise SystemExit("BYTES must be a multiple of 4")
words = n // 4
out = [".data", ".globl _bigk_start", ".globl _bigk_end", ".globl _bigk_words", "_bigk_start:"]
for i in range(0, words, 8):
    out.append("\t.long " + ",".join("0x%08x" % ((j * 2654435761 + 0x1234567) & 0xFFFFFFFF)
                                      for j in range(i, min(i + 8, words))))
out.append("_bigk_end:")
out.append("_bigk_words:\t.long %d" % words)
open(sys.argv[2], "w").write("\n".join(out) + "\n")
print("bigk: %d bytes (%d words)" % (n, words))
