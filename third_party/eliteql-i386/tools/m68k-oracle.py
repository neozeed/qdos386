import struct
from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN
from unicorn import m68k_const as m
class Oracle:
 def __init__(self,path):
  b=open(path,'rb').read();h=struct.unpack_from('>8I',b);t,d,z,ns,e,tr,dr=h[1:];base=0x100000
  body=bytearray(b[32:32+t+d]);o=32+t+d
  for p in range(o,o+tr+dr,8):
   off,r=struct.unpack_from('>II',b,p);off += t if p>=o+tr else 0;length=(r>>5)&3;ext=(r>>4)&1;pc=(r>>7)&1
   assert not ext,'unresolved reference in oracle'
   if not pc:
    n=1<<length;v=int.from_bytes(body[off:off+n],'big');body[off:off+n]=((v+base)&((1<<(n*8))-1)).to_bytes(n,'big')
  o+=tr+dr;st=b[o+ns:];self.symbols={}
  for i in range(ns//12):
   ix,ty,ot,de,v=struct.unpack_from('>IBBHI',b,o+i*12);self.symbols[st[ix:].split(b'\0')[0].decode()]=base+v
  self.u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);self.u.ctl_set_cpu_model(m.UC_CPU_M68K_M68000);self.u.mem_map(0x100000,0x300000);self.u.mem_write(base,bytes(body))
 def call(self,name,args):
  sp=0x2fff00;self.u.mem_write(sp,struct.pack('>'+str(len(args)+1)+'I',0x1ff000,*(a&0xffffffff for a in args)));self.u.reg_write(m.UC_M68K_REG_A7,sp)
  self.u.emu_start(self.symbols[name],0x1ff000,count=1000000)
  assert self.u.reg_read(m.UC_M68K_REG_PC)==0x1ff000
  return self.u.reg_read(m.UC_M68K_REG_D0)
if __name__=='__main__':
 import sys
 a=Oracle(sys.argv[1]);a.u.mem_write(0x300000,bytes(36));a.call('_own_rotate',[0x300000]);print('original m68k own_rotate executed')
