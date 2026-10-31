#!/usr/bin/env python3
from oracle_pair import *
r=random.Random(0x8191191);digest=hashlib.sha256()
for i in range(2048):
 coords=[r.randint(-8191,8191) for _ in range(4)]
 if i%2==0:coords=[r.randint(-20,275),r.randint(-20,211),r.randint(-20,275),r.randint(-20,211)]
 frame=bytes(32768)
 for cpu in [m.u,u]:cpu.mem_write(0x310000,bytes([0x5a])*32+frame+bytes([0xa5])*32)
 args=[0x310020]+[v&0xffffffff for v in coords]
 m.call('_xor_line',args);call('xor_line',args)
 a=bytes(m.u.mem_read(0x310000,32832));b=bytes(u.mem_read(0x310000,32832))
 assert a==b,(i,coords,'frame mismatch')
 digest.update(a)
 m.call('_xor_line',args);call('xor_line',args)
 for cpu in [m.u,u]:assert bytes(cpu.mem_read(0x310020,32768))==frame,(i,'XOR round trip')
 for cpu in [m.u,u]:assert bytes(cpu.mem_read(0x310000,32))==bytes([0x5a])*32 and bytes(cpu.mem_read(0x318020,32))==bytes([0xa5])*32
print('MODE 8 raster: PASS (2048 original-68000 frames, clipped/boundary lines, XOR round trips, guards)')
print('SHA256',digest.hexdigest())
