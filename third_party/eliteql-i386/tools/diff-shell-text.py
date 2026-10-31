#!/usr/bin/env python3
from oracle_pair import *
from unicorn import m68k_const as mc
m=Oracle(root/'oracle/shell-m68k.o');m.u.mem_map(0x20000,0x9000)
r=random.Random(0x3865e11);digest=hashlib.sha256();dst=0x310000;string=0x300000
# The original internal routines take registers, not cdecl arguments.
def original(name,registers):
 for reg,value in registers:m.u.reg_write(reg,value&0xffffffff)
 m.call(name,[])
for i in range(2048):
 data=r.randbytes(32768)
 text=bytes(r.randrange(32,128) for j in range(r.randrange(1,90)))+b'\0'
 y=r.choice([0,185,186,65535,r.randrange(192)])
 x0=r.choice([0,251,252,65535,r.randrange(256)])
 m.u.mem_write(0x20000,data);u.mem_write(dst-16,b'G'*16+data+b'H'*16)
 for em in (m.u,u):em.mem_write(string,text)
 default=i%2==0
 original('asm_text' if default else 'asm_text_at',[(mc.UC_M68K_REG_A4,string),(mc.UC_M68K_REG_D6,y),(mc.UC_M68K_REG_D7,x0)])
 call('asm_text' if default else 'asm_text_at',[dst,string,y]+([] if default else [x0]))
 a=bytes(m.u.mem_read(0x20000,32768));b=bytes(u.mem_read(dst,32768))
 assert a==b,('text',i,x0,y,next(j for j,(a,b) in enumerate(zip(a,b)) if a!=b))
 assert u.mem_read(dst-16,16)==b'G'*16 and u.mem_read(dst+32768,16)==b'H'*16
 digest.update(a)
 original('asm_clear_view',[]);call('asm_clear_view',[dst])
 assert m.u.mem_read(0x20000,32768)==u.mem_read(dst,32768)
 assert u.mem_read(dst,24576)==bytes(24576) and u.mem_read(dst+24576,8192)==data[24576:]
for i in range(4096):
 value=r.getrandbits(32);width=r.randrange(1,11)
 for em in (m.u,u):em.mem_write(string-16,b'G'*16+bytes(10)+b'H'*16)
 original('asm_decimal',[(mc.UC_M68K_REG_A0,string),(mc.UC_M68K_REG_D0,value),(mc.UC_M68K_REG_D1,width)])
 result=call('asm_decimal',[string,value,width])
 assert result==m.u.reg_read(mc.UC_M68K_REG_D0),('decimal quotient',i)
 a=bytes(m.u.mem_read(string-16,42));b=bytes(u.mem_read(string-16,42));assert a==b,('decimal',i,value,width);digest.update(a)
print('PASS: original shell font/text 2048 complete frames + viewport clears; 4096 decimal values/widths; i386 ABI and guards; SHA256',digest.hexdigest())

for i in range(2048):
 data=bytearray(r.randbytes(148));data[:4]=b'EQA1' if i%2 else b'EQA2'
 m.u.mem_write(m.symbols['save_buffer'],bytes(data));u.mem_write(dst,bytes(data))
 original('save_checksum',[]);value=call('save_checksum',[dst])
 assert value==m.u.reg_read(mc.UC_M68K_REG_D0),('checksum',i)
 digest.update(struct.pack('<I',value))
 for j,length in enumerate([10,22,64,34,1,4]):
  address=string+256*j;payload=r.randbytes(length)
  for em in (m.u,u):em.mem_write(address-8,b'G'*8+payload+b'H'*8)
  m.u.mem_write(m.symbols['save_parts']+j*6,struct.pack('>IH',address,length))
  u.mem_write(string+2048+j*6,struct.pack('<IH',address,length))
 restore=i%2
 original('save_copy',[(mc.UC_M68K_REG_D6,restore)])
 call('save_copy',[dst,string+2048,restore])
 assert m.u.mem_read(m.symbols['save_buffer'],148)==u.mem_read(dst,148),('save buffer',i)
 for j,length in enumerate([10,22,64,34,1,4]):
  a=bytes(m.u.mem_read(string+256*j-8,length+16));b=bytes(u.mem_read(string+256*j-8,length+16))
  assert a==b,('save payload',i,j);digest.update(a)
print('PASS: 2048 original shell save checksums and copy/restore cases, payloads/guards; combined SHA256',digest.hexdigest())
