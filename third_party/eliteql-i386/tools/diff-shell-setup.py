#!/usr/bin/env python3
from oracle_pair import *
m=Oracle(root/'oracle/full-shell-m68k.o');m.u.mem_map(0x20000,0x9000)
bm=(root/'oracle/full-shell-m68k.o').read_bytes();bm_h=struct.unpack_from('>8I',bm)
bn=(root/'build/aout/shell-audit').read_bytes();bn_h=struct.unpack_from('<8I',bn)
mbase=bytes(m.u.mem_read(0x100000,sum(bm_h[1:4])));nbase=bytes(u.mem_read(0x10000,sum(bn_h[1:4])))
r=random.Random(0x53ce386);digest=hashlib.sha256();frame=0x310000
for case in range(64):
 m.u.mem_write(0x100000,mbase);u.mem_write(0x10000,nbase)
 u.mem_write(sy['elite_screen'],struct.pack('<I',frame))
 params={'asm_count':[1,3,12,12][case%4],'asm_mode':case%4,'asm_salvage':(case//4)%4,'asm_model':case%10}
 for name,value in params.items():
  m.u.mem_write(m.symbols[name],struct.pack('>H',value));u.mem_write(sy[name],struct.pack('<H',value))
 controls=bytes([r.randrange(256),r.randrange(256)])
 for em,syms in [(m.u,m.symbols),(u,sy)]:em.mem_write(syms['asm_controls']+2,controls)
 data=r.randbytes(32768);m.u.mem_write(0x20000,data);u.mem_write(frame,data)
 m.call('asm_setup',[]);call('asm_setup',[])
 for name,size in [('asm_vectors',432),('asm_controls',13),('asm_params',4),('asm_status',5),('asm_commander',10),('asm_travel',64),('asm_market',34),('asm_player',4),('asm_equipment',5),('asm_laser',12),('asm_compass',4),('asm_dust',168)]:
  a=bytes(m.u.mem_read(m.symbols[name],size));b=bytes(u.mem_read(sy[name],size))
  assert a==b,(case,name,next((j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y));digest.update(a)
 a=bytes(m.u.mem_read(0x20000,32768));b=bytes(u.mem_read(frame,32768));assert a==b,(case,'frame',next((j,x,y) for j,(x,y) in enumerate(zip(a,b)) if x!=y));digest.update(a)
 for j in range(12):
  a=bytes(m.u.mem_read(m.symbols['asm_ships']+272*j+4,268));b=bytes(u.mem_read(sy['asm_ships']+272*j+4,268))
  assert struct.unpack('>6h',a[:12])==struct.unpack('<6h',b[:12]),(case,'cache header',j)
  assert a[12:]==b[12:],(case,'cache bytes',j)
print('PASS: 64 original full-shell asm_setup scenarios; all ship INWKs, controls, serialized commander/travel/market, RNG/dust, caches and 32KB frames; SHA256',digest.hexdigest())
