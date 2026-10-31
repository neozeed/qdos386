#!/usr/bin/env python3
"""Execute original 68000 and translated i386 routines on identical bytes."""
import importlib,struct,subprocess,random,hashlib
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn import x86_const as x
root=Path(__file__).resolve().parent.parent
Oracle=importlib.import_module('m68k-oracle').Oracle
m=Oracle(root/'oracle/core-m68k.o')
import os
native_path=root/os.environ.get('ELITE_NATIVE_IMAGE','build/elf/flight_test')
b=native_path.read_bytes();u=Uc(UC_ARCH_X86,UC_MODE_32);sy={}
if b[:4]==b'\x7fELF':
 h=struct.unpack_from('<16sHHIIIIIHHHHHH',b);pages=set()
 for i in range(h[10]):
  t,o,va,pa,fs,ms,f,a=struct.unpack_from('<IIIIIIII',b,h[5]+i*h[9])
  if t!=1:continue
  for p in range(va&~4095,(va+ms+4095)&~4095,4096):
   if p not in pages:u.mem_map(p,4096);pages.add(p)
  u.mem_write(va,b[o:o+fs])
 for line in subprocess.check_output(['nm',str(native_path)],text=True).splitlines():
  fields=line.split()
  if len(fields)==3:sy[fields[2]]=int(fields[0],16)
else:
 magic,t,d,z,ns,entry,tr,dr=struct.unpack_from('<8I',b)
 assert magic==0x107 and tr==dr==0
 base=0x10000
 u.mem_map(base,(t+d+z+4095)&~4095);u.mem_write(base,b[32:32+t+d])
 o=32+t+d;strings=b[o+ns:]
 for i in range(ns//12):
  ix,ty,ot,de,v=struct.unpack_from('<IBBHI',b,o+i*12)
  assert ty!=1, 'unresolved historical-link symbol'
  name=strings[ix:].split(b'\0')[0].decode()
  if name.startswith('_'):name=name[1:]
  sy[name]=v
u.mem_map(0x200000,0x200000)
def call(name,args):
 sp=0x2fff00;u.mem_write(sp,struct.pack('<'+str(len(args)+1)+'I',0x2ff000,*(a&0xffffffff for a in args)));u.reg_write(x.UC_X86_REG_ESP,sp)
 sentinels=[(x.UC_X86_REG_EBX,0x12345678),(x.UC_X86_REG_ESI,0x23456789),(x.UC_X86_REG_EDI,0x3456789a),(x.UC_X86_REG_EBP,0x456789ab)]
 for reg,value in sentinels:u.reg_write(reg,value)
 u.emu_start(sy[name],0x2ff000,count=1000000)
 assert u.reg_read(x.UC_X86_REG_EIP)==0x2ff000
 for reg,value in sentinels:assert u.reg_read(reg)==value,(name,'callee-save register')
 assert u.reg_read(x.UC_X86_REG_ESP)==sp+4
 return u.reg_read(x.UC_X86_REG_EAX)
