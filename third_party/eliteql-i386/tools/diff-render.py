#!/usr/bin/env python3
"""RC6 original 68000 vs actual GAS1.38 i386 renderer and circle frames."""
from oracle_pair import *
r=random.Random(0x386c1ec1);digest=hashlib.sha256();screen=0x310000;obj=0x300100;mat=0x300400;wk=0x300500
for i in range(2048):
 model=i%10
 mi=struct.unpack('>I',m.u.mem_read(m.symbols['_models']+4*model,4))[0]
 ni=struct.unpack('<I',u.mem_read(sy['models']+4*model,4))[0]
 z=r.choice([3,4,255,256,4095,4096,32767,r.randrange(4,20000)])
 pos=[r.randrange(-z,z+1),r.randrange(-z,z+1),z]
 # Original native renderer accepts a caller-supplied signed matrix.
 matrix=[r.randrange(-256,257) for _ in range(9)]
 for em,order,ptr,glob in [(m.u,'>',mi,m.symbols),(u,'<',ni,sy)]:
  em.mem_write(screen-16,b'G'*16+bytes(32768)+b'H'*16)
  em.mem_write(obj,struct.pack(order+'I6h',ptr,*pos,0,0,0)+bytes(256))
  em.mem_write(mat,struct.pack(order+'9h',*matrix))
  em.mem_write(wk,bytes([0]*31+[64 if i%3==0 else 0]+[0]*4))
  em.mem_write(glob['_live_matrix' if em is m.u else 'live_matrix'],struct.pack(order+'I',mat))
  em.mem_write(glob['_render_inwk' if em is m.u else 'render_inwk'],struct.pack(order+'I',wk))
  em.mem_write(glob['_render_exploding' if em is m.u else 'render_exploding'],struct.pack(order+'H',i%7==0))
 m.call('_render_ship',[screen,obj]);call('render_ship',[screen,obj])
 a=bytes(m.u.mem_read(screen-16,32800));b=bytes(u.mem_read(screen-16,32800))
 assert a==b,('render pixels',i,next(j for j,(x,y) in enumerate(zip(a,b)) if x!=y))
 ac=struct.unpack('>H',m.u.mem_read(obj+12,2))[0];bc=struct.unpack('<H',u.mem_read(obj+12,2))[0]
 assert ac==bc,('line count',i,ac,bc)
 assert m.u.mem_read(obj+16,ac*4)==u.mem_read(obj+16,bc*4),('line cache',i)
 assert m.u.mem_read(wk,36)==u.mem_read(wk,36),('beam state',i)
 av=struct.unpack('>H',m.u.mem_read(m.symbols['_effect_visible'],2))[0];bv=struct.unpack('<H',u.mem_read(sy['effect_visible'],2))[0]
 assert av==bv,('visibility',i)
 # Original scratch projection array retains nonprojected vertices.
 av=struct.unpack('>64h',m.u.mem_read(m.symbols['_effect_vertices'],128));bv=struct.unpack('<64h',u.mem_read(sy['effect_vertices'],128))
 assert av==bv,('vertices',i,av,bv)
 digest.update(a)
 m.call('_render_cache',[screen,obj]);call('render_cache',[screen,obj])
 assert m.u.mem_read(screen,32768)==bytes(32768) and u.mem_read(screen,32768)==bytes(32768),('erase',i)
for i in range(512):
 args=[screen,obj,r.randrange(-255,512),r.randrange(-191,384),r.randrange(1,249),r.choice([2,4,8])]
 for em in [m.u,u]:em.mem_write(screen-16,b'G'*16+bytes(32768)+b'H'*16);em.mem_write(obj,bytes(256))
 m.call('_circle_xy',args);call('circle_xy',args)
 a=bytes(m.u.mem_read(screen-16,32800));b=bytes(u.mem_read(screen-16,32800))
 assert a==b,('circle pixels',i,args)
 digest.update(a)
print('PASS: 2048 original 68000 ship renders / vertices / line caches / erase; 512 clipped circles; i386 ABI; SHA256',digest.hexdigest())
