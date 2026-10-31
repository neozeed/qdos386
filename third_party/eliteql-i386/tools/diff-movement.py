#!/usr/bin/env python3
from oracle_pair import *
r=random.Random(0x38668000);digest=hashlib.sha256();count=0
for name in ['own_rotate','move_ship','rotate_body','tidy_ship','motion_position','motion_matrix']:
 for i in range(2048):
  ship=bytes(r.randrange(256) for _ in range(36));params=bytes([r.randrange(256),r.randrange(256),r.randrange(41),r.randrange(41)])
  if i%2==0:
   q=bytearray(ship)
   for j in [2,5,8]:q[j]&=128
   ship=bytes(q)
  for cpu in [m.u,u]:
   cpu.mem_write(0x300000,bytes([85])+ship+bytes([170]))
   cpu.mem_write(0x300100,params)
   cpu.mem_write(0x300200,bytes([0x5a])*32)
  args=[0x300001]
  if name in ['move_ship','rotate_body']:args.append(0x300100)
  if name in ['motion_position','motion_matrix']:args.append(0x300200)
  a=m.call('_'+name,args);c=call(name,args)
  original=bytes(m.u.mem_read(0x300000,38));native=bytes(u.mem_read(0x300000,38))
  assert original==native,(name,i,'state',ship.hex(),params.hex(),original.hex(),native.hex())
  if name=='motion_position':assert a==c,(name,i,'result',a,c)
  if name in ['motion_position','motion_matrix']:
   n=3 if name=='motion_position' else 9
   # Native C short[] is the documented endian adaptation.
   aa=bytes(m.u.mem_read(0x300200,n*2));cc=bytes(u.mem_read(0x300200,n*2))
   assert aa==b''.join(cc[j:j+2][::-1] for j in range(0,n*2,2)),(name,i,'short array',aa.hex(),cc.hex())
   assert bytes(m.u.mem_read(0x300200+n*2,32-n*2))==bytes(u.mem_read(0x300200+n*2,32-n*2))
  digest.update(original);count+=1
 print(name+': PASS (2048 original-68000 comparisons, i386 ABI guards)')
print('TOTAL',count,'SHA256',digest.hexdigest())
