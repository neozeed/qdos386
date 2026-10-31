#!/usr/bin/env python3
"""Test-only GAS 1.38 i386 OMAGIC -> ELF32 relocatable adapter.
Preserves instructions; adjusts relocation addends and symbol conventions.
Not part of the game or the QDOS runtime. Unsupported records fail closed.
"""
import struct,sys
from pathlib import Path
b=Path(sys.argv[1]).read_bytes()
magic,t,d,z,ns,entry,nt,nd=struct.unpack_from('<8I',b)
assert magic==0x107 and entry==0 and ns%12==0 and nt%8==nd%8==0
text=bytearray(b[32:32+t]);data=bytearray(b[32+t:32+t+d])
o=32+t+d+nt+nd;strings=b[o+ns:]
assert struct.unpack_from('<I',strings)[0]==len(strings)
raw=[]
for i in range(ns//12):
 ix,ty,other,desc,v=struct.unpack_from('<IBBHI',b,o+i*12)
 assert ty in range(10) and not other and not desc
 name=strings[ix:].split(b'\0')[0].decode()
 if name.startswith('_'):name=name[1:]
 sec={0:0,2:0xfff1,4:1,6:2,8:3}[ty&~1]
 base={0:0,2:0,4:0,6:t,8:t+d}[ty&~1]
 assert sec or v==0,'common symbols unsupported'
 raw.append((i,name,v-base,sec,ty&1))
strtab=bytearray(b'\0');symbols=[bytes(16)]
for section in (1,2,3):symbols.append(struct.pack('<IIIBBH',0,0,0,3,0,section))
remap={};ordered=sorted(raw,key=lambda x:x[4]);firstglobal=4+sum(not x[4] for x in raw)
for old,name,v,sec,glob in ordered:
 remap[old]=len(symbols);ix=len(strtab);strtab+=name.encode()+b'\0'
 symbols.append(struct.pack('<IIIBBH',ix,v,0,glob<<4,0,sec))
def reloc(buf,ro,count,placebase):
 out=bytearray()
 for i in range(count//8):
  p,r=struct.unpack_from('<II',b,ro+i*8)
  sym=r&0xffffff;pc=(r>>24)&1;length=(r>>25)&3;ext=(r>>27)&1
  assert r>>28==0 and length==2 and p+4<=len(buf)
  a=struct.unpack_from('<I',buf,p)[0]
  if pc:a+=placebase+p
  if ext:es=remap[sym]
  else:
   assert sym in (4,6,8)
   es={4:1,6:2,8:3}[sym];a-={4:0,6:t,8:t+d}[sym]
  struct.pack_into('<I',buf,p,a&0xffffffff)
  out+=struct.pack('<II',p,(es<<8)|(2 if pc else 1))
 return out
rt=reloc(text,32+t+d,nt,0);rd=reloc(data,32+t+d+nt,nd,t)
names=['','.text','.data','.bss','.rel.text','.rel.data','.symtab','.strtab','.shstrtab','.note.GNU-stack']
shstr=bytearray(b'\0');ni=[0]
for name in names[1:]:ni.append(len(shstr));shstr+=name.encode()+b'\0'
sections=[(0,0,b'',0,0,0,0,0),(1,6,text,t,0,0,4,0),(1,3,data,d,0,0,4,0),(8,3,b'',z,0,0,4,0),
 (9,0,rt,len(rt),6,1,4,8),(9,0,rd,len(rd),6,2,4,8),
 (2,0,b''.join(symbols),16*len(symbols),7,firstglobal,4,16),
 (3,0,strtab,len(strtab),0,0,1,0),(3,0,shstr,len(shstr),0,0,1,0),(1,0,b'',0,0,0,1,0)]
output=bytearray(52);headers=[]
for i,(typ,flags,payload,size,link,info,align,ent) in enumerate(sections):
 while len(output)%max(align,1):output.append(0)
 off=len(output);output+=payload
 headers.append(struct.pack('<10I',ni[i],typ,flags,0,off,size,link,info,align,ent) if i else bytes(40))
while len(output)%4:output.append(0)
shoff=len(output);output+=b''.join(headers)
output[:52]=struct.pack('<16sHHIIIIIHHHHHH',b'\x7fELF\x01\x01\x01'+bytes(9),1,3,1,0,0,shoff,0,52,0,0,40,len(sections),8)
Path(sys.argv[2]).write_bytes(output)
