#!/usr/bin/env python3
from pathlib import Path
import re
root=Path(__file__).resolve().parent.parent
src=(root/'reference/src/shell.s').read_text()
blocks=[]
for a,b in [('asm_labels','label1'),('asm_clear_view','asm_restore_view'),('asm_decimal','asm_commander_page')]:
 blocks.append(src[src.index(a+':\n'):src.index(b+':')])
blocks.append('asm_font:\n'+src.split('asm_font:\n',1)[1].split('.sect',1)[0])
blocks.append(src[src.index('save_copy:\n'):src.index('.sect .data',src.index('save_copy:\n'))])
blocks.append('.sect .data\nsave_parts: .space 36\nsave_buffer: .space 148\n')
s='.sect .text\n'+'\n'.join(blocks)
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
(root/'oracle/shell-m68k.s').write_text(s+'\n')
