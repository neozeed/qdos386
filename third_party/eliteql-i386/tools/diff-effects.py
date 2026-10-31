#!/usr/bin/env python3
from oracle_pair import *
r=random.Random(0x68000386);digest=hashlib.sha256();state=0x300100;params=0x300300;rng=0x300400;frame=0x310000;cache=0x300600
for i in range(1024):
 s=bytearray(r.randbytes(168));s[114:]=bytes(54)
 p=bytes([r.randrange(256),r.randrange(256),r.randrange(1,41)])
 seed=r.randbytes(4);mode=r.choice([0,1,2,3,4]);view=r.randrange(4)
 for em in [m.u,u]:
  em.mem_write(state-8,b'G'*8+bytes(s)+b'H'*8);em.mem_write(params,p);em.mem_write(rng,seed);em.mem_write(frame-8,b'G'*8+bytes(32768)+b'H'*8)
 args=[state,params,rng,view,mode,frame]
 for step in range(3):
  m.call('_dust_run',args);call('dust_run',args)
  for address,size in [(state-8,184),(rng,4),(frame-8,32784)]:
   a=bytes(m.u.mem_read(address,size));b=bytes(u.mem_read(address,size))
   assert a==b,('dust',i,mode,view,step,hex(address),next(j for j,(x,y) in enumerate(zip(a,b)) if x!=y))
   digest.update(a)
for i in range(1024):
 # Header supports up to 12 projected vertices (4-byte original LE coordinates).
 heap=bytearray(r.randbytes(56));heap[2]=r.randrange(1,13)*4+6
 for j in range(7,55,4):heap[j:j+4]=struct.pack('<hh',r.randrange(-200,456),r.randrange(-200,392))
 seed=r.randbytes(4);planet=r.randrange(256)
 for em in [m.u,u]:em.mem_write(cache,bytes(heap));em.mem_write(rng,seed);em.mem_write(state-8,b'G'*8+bytes(384)+b'H'*8)
 a=m.call('_cloud_points',[cache,state,rng,planet]);b=call('cloud_points',[cache,state,rng,planet]);assert a==b,('points count',i,a,b)
 for addr,size in [(rng,4),(state-8,400)]:
  av=bytes(m.u.mem_read(addr,size));bv=bytes(u.mem_read(addr,size));assert av==bv,('points',i,hex(addr));digest.update(av)
for i in range(64):
 tr=bytearray(r.randbytes(64));tr[9]=r.randrange(71);cursor=r.randbytes(2)
 for em in [m.u,u]:em.mem_write(state,bytes(tr));em.mem_write(params,cursor);em.mem_write(frame-8,b'G'*8+bytes(32768)+b'H'*8)
 m.call('_galaxy_chart_cursor',[frame,state,params]);call('galaxy_chart_cursor',[frame,state,params])
 a=bytes(m.u.mem_read(frame-8,32784));b=bytes(u.mem_read(frame-8,32784));assert a==b,('chart',i);digest.update(a)
print('PASS: 1024 dust scenarios x3 steps (all 168 state bytes, RNG, full framebuffer, guards); 1024 cloud point/RNG/guards; 64 complete galaxy charts; SHA256',digest.hexdigest())
