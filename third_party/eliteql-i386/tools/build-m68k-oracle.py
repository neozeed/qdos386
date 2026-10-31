#!/usr/bin/env python3
"""Syntax-only JAS -> historical m68k GAS conversion for the reference oracle.
No source algorithms changed. Original files remain untouched.
"""
from pathlib import Path
import re,sys
root=Path(__file__).resolve().parent.parent
names=sys.argv[1:] or ['motion','flight','tidy']
s='\n'.join((root/'reference/src'/(n+'.s')).read_text() for n in names)
s='\n'.join(x.split(';')[0] for x in s.splitlines())
for a,b in [('.sect .rom','.data'),('.sect .text','.text'),('.sect .data','.data'),('.sect .bss','.data'),('.data1','.byte'),('.data2','.word'),('.data4','.long')]:s=s.replace(a,b)
s=re.sub(r'\bblo\b','bcs',s)
s=re.sub(r'\bbhs\b','bcc',s)
s=re.sub(r'(?m)^\s*\.extern[^\n]*','',s)
s=re.sub(r'(?m)^(\s*)(\w+)\.([bwl])\b',r'\1\2\3',s)
# Use absolute relocation for label(pc): legacy GAS's MIT PC-relative
# syntax otherwise places this oracle's effective address two bytes early.
s=re.sub(r'([A-Za-z_]\w*(?:[+-]\d+)?)\(pc\)',r'\1',s)
s=re.sub(r'-\((a[0-7]|sp)\)',r'\1@-',s)
s=re.sub(r'\((a[0-7]|sp)\)\+',r'\1@+',s)
s=re.sub(r'([^\s,()]*)\((a[0-7]|sp|pc)(?:,(d[0-7]|a[0-7])\.([wl]))?\)',lambda m:m[2]+'@'+('('+ (m[1] or '0')+ (','+m[3]+':'+m[4] if m[3] else '')+')' if m[1] or m[3] else ''),s)
(root/'oracle').mkdir(exist_ok=True)
(root/'oracle/core-m68k.s').write_text(s+'\n')
