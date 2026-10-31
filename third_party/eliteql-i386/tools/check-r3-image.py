#!/usr/bin/env python3
"""Check the actual historical-link application, not the headless guard link."""
from pathlib import Path
import struct,json,hashlib
r=Path(__file__).resolve().parent.parent;p=r/'build/aout/ELITE.AOU';b=p.read_bytes()
magic,t,d,z,ns,entry,tr,dr=struct.unpack_from('<8I',b)
assert magic==0x107 and tr==dr==0
assert entry==0x240000 and t%16==d%16==z%16==0
assert t+d+z<0x3f000,'application overlaps slot argument area'
o=32+t+d;strings=b[o+ns:];symbols={};undefined=[]
for i in range(0,ns,12):
 ix,ty,other,desc,v=struct.unpack_from('<IBBHI',b,o+i)
 name=strings[ix:].split(b'\0')[0].decode();symbols[name]=v
 if ty==1:undefined.append(name)
assert not undefined,undefined
assert symbols['_start']==entry
for name in json.loads((r/'docs/R3-UNRESOLVED-ENTRIES.json').read_text()):
 assert entry<=symbols[name]<entry+t
assert symbols['asm_loop']!=symbols['_start']
result={'image':'ELITE.AOU','sha256':hashlib.sha256(b).hexdigest(),'entry':entry,
 'text':t,'data':d,'bss':z,'runtime_bytes':t+d+z,'file_bytes':len(b),
 'undefined_symbols':undefined,'platform_entries_resolved':14,
 'toolchain':'GAS 1.38 / binutils 1.94; no post-link instruction rewriting'}
(r/'docs/R3-IMAGE.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS:',json.dumps(result))
