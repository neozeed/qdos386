#!/usr/bin/env python3
"""Fail a build if old GAS silently stopped before any source label."""
import re,struct,sys
from pathlib import Path
s=Path(sys.argv[1]).read_text();b=Path(sys.argv[2]).read_bytes()
h=struct.unpack_from('<8I',b);t,d,z,ns,e,tr,dr=h[1:];o=32+t+d+tr+dr;strings=b[o+ns:]
found=set()
for i in range(ns//12):
 ix,ty,ot,de,v=struct.unpack_from('<IBBHI',b,o+i*12)
 if ty&~1:found.add(strings[ix:].split(b'\0')[0].decode())
expected=set(re.findall(r'(?m)^\s*([A-Za-z_][\w.]*):',s))
missing=expected-found
if missing:raise SystemExit('GAS object omits source labels: '+', '.join(sorted(missing)))
