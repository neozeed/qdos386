#!/usr/bin/env python3
"""Run freestanding ELF32 fixture tests with Unicorn; only write/exit syscalls.
No game functions or arithmetic are provided by this harness.
"""
import struct,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_INTR
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP
b=open(sys.argv[1],'rb').read()
assert b[:7]==b'\x7fELF\x01\x01\x01'
h=struct.unpack_from('<16sHHIIIIIHHHHHH',b)
u=Uc(UC_ARCH_X86,UC_MODE_32)
pages=set()
for i in range(h[10]):
 t,off,va,pa,fs,ms,flags,align=struct.unpack_from('<IIIIIIII',b,h[5]+i*h[9])
 if t!=1:continue
 for p in range(va&~4095,(va+ms+4095)&~4095,4096):
  if p not in pages:u.mem_map(p,4096);pages.add(p)
 u.mem_write(va,b[off:off+fs])
u.mem_map(0x70000000,0x100000)
u.reg_write(UC_X86_REG_ESP,0x700ffff0)
result=[]
def intr(u,n,data):
 if n!=128:raise RuntimeError('unexpected interrupt '+str(n))
 call=u.reg_read(UC_X86_REG_EAX)
 if call==1:result.append(u.reg_read(UC_X86_REG_EBX));u.emu_stop()
 elif call==4:
  assert u.reg_read(UC_X86_REG_EBX)==1
  data=bytes(u.mem_read(u.reg_read(UC_X86_REG_ECX),u.reg_read(UC_X86_REG_EDX)))
  sys.stdout.buffer.write(data);sys.stdout.buffer.flush();u.reg_write(UC_X86_REG_EAX,len(data))
 else:raise RuntimeError('unexpected syscall '+str(call))
u.hook_add(UC_HOOK_INTR,intr)
u.emu_start(h[4],0,count=500000000)
if not result:raise RuntimeError('instruction limit reached without exit')
sys.exit(result[0])
