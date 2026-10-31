#!/usr/bin/env python3
"""Link partial native shell for bounded oracle tests.
Every unresolved entry becomes an INT3 failure guard in TEST-ONLY code. No
untranslated path can silently return success. This is not a game executable.
"""
from pathlib import Path
import re,struct,subprocess,json
r=Path(__file__).resolve().parent.parent
mods=re.search(r'^MODULES = (.*)$',(r/'Makefile').read_text(),re.M)[1].split()
mods+=['shell_text','shell_save','shell_data','shell_cheats','shell_start','shell_setup','shell_scene','shell_flow','shell_sound','shell_universe','shell_adapters','shell_spawn','shell_world']
subprocess.run(['make','-C',str(r),*[f'build/aout/{m}.o' for m in mods],'build/tools/ld194'],check=True)
objs=[str(r/f'build/aout/{m}.o') for m in mods]
ld=str(r/'build/tools/ld194');linked=r/'build/aout/shell-partial.o'
subprocess.run([ld,'-r','-o',str(linked),*objs],check=True)
b=linked.read_bytes();h=struct.unpack_from('<8I',b);t,d,z,ns,e,tr,dr=h[1:];o=32+t+d+tr+dr;st=b[o+ns:];missing=[]
for i in range(ns//12):
 ix,ty,ot,de,v=struct.unpack_from('<IBBHI',b,o+i*12)
 if ty==1:missing.append(st[ix:].split(b'\0')[0].decode())
s=['# Test-only hard-failure guards for unimplemented paths.','.text']
for name in missing:s+=['.globl '+name,name+':',' int $3']
s+=['.globl _headless_start','_headless_start:',' ret']
(r/'build/aout/shell-guards.s').write_text('\n'.join(s)+'\n')
subprocess.run([str(r/'build/tools/as138'),'-o',str(r/'build/aout/shell-guards.o'),str(r/'build/aout/shell-guards.s')],check=True)
subprocess.run([ld,'-N','-e','_headless_start','-Ttext','0x10000','-o',str(r/'build/aout/shell-audit'),*objs,str(r/'build/aout/shell-guards.o')],check=True)
(r/'docs/R3-UNRESOLVED-ENTRIES.json').write_text(json.dumps(missing,indent=2)+'\n')
print(len(missing),'unimplemented/platform entries guarded with INT3; cannot be treated as a playable link')
