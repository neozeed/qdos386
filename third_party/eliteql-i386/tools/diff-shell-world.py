#!/usr/bin/env python3
"""Original warp/encounter orchestration and every existing cheat action."""
from oracle_pair import *
from unicorn import m68k_const as mr
m=Oracle(root/'oracle/full-shell-m68k.o');m.u.mem_map(0x20000,0x9000)
u.mem_write(sy['elite_screen'],struct.pack('<I',0x310000))
r=random.Random(0x386138);digest=hashlib.sha256()
def put(name,data):
 m.u.mem_write(m.symbols[name],data);u.mem_write(sy[name.lstrip('_')],data)
def word(name,v):
 m.u.mem_write(m.symbols[name],struct.pack('>H',v));u.mem_write(sy[name],struct.pack('<H',v))
def invoke(name):
 m.call(name,[])
 sp=0x2fff00;u.mem_write(sp,struct.pack('<I',0x2ff000));u.reg_write(x.UC_X86_REG_ESP,sp)
 u.emu_start(sy[name],0x2ff000,count=3000000)
 assert u.reg_read(x.UC_X86_REG_EIP)==0x2ff000,name
def equal(name,n):
 a=bytes(m.u.mem_read(m.symbols[name],n));b=bytes(u.mem_read(sy[name.lstrip('_')],n))
 assert a==b,(case,action,name,next((i,x,y) for i,(x,y) in enumerate(zip(a,b)) if x!=y));digest.update(a)
def integer(name,n):
 a=int.from_bytes(m.u.mem_read(m.symbols[name],n),'big');b=int.from_bytes(u.mem_read(sy[name],n),'little')
 assert a==b,(case,action,name,a,b);digest.update(a.to_bytes(n,'big'))
for case in range(128):
 action='setup';word('asm_mode',3);word('asm_count',12);word('asm_salvage',2);put('flow_config',bytes([255,0,0]));invoke('asm_setup')
 # Original overflow saturation and already-installed equipment cases.
 put('asm_commander',struct.pack('>I', [0,1000,0xfffffff0,r.getrandbits(32)][case%4])+r.randbytes(6))
 put('asm_equipment',r.randbytes(5))
 for action in ['cheat_cash','cheat_missiles','cheat_computer','cheat_bomb','cheat_ecm','cheat_drive']:
  invoke(action)
  for name,n in [('asm_commander',10),('asm_equipment',5),('asm_player',4),('asm_travel',64)]:equal(name,n)
 # Run the actual shell scan/decision path with both universe representations.
 action='warp';word('asm_world',case%2);put('asm_travel',bytes(m.u.mem_read(m.symbols['asm_travel'],64)))
 m.u.mem_write(m.symbols['asm_travel']+11,bytes([case%3==0]));u.mem_write(sy['asm_travel']+11,bytes([case%3==0]))
 if case%4==1:
  # No mass lock, both bodies outside the original proximity threshold.
  v=bytearray(m.u.mem_read(m.symbols['asm_vectors'],432))
  for j in range(12):v[j*36+31]=160
  put('asm_vectors',bytes(v));put('asm_station',bytes(4))
  for name in ['asm_planet','asm_placeholder']:
   v=bytearray(m.u.mem_read(m.symbols[name],36));v[8]=128;put(name,bytes(v))
  m.u.mem_write(m.symbols['asm_travel']+11,b'\0');u.mem_write(sy['asm_travel']+11,b'\0')
 invoke('asm_warp')
 for name,n in [('asm_planet',36),('asm_placeholder',36),('asm_dust',168),('asm_enc_params',8)]:equal(name,n)
 for name,n in [('asm_warp_result',2),('asm_warp_timer',2),('asm_warps',4)]:integer(name,n)
 action='encounters';word('asm_ambient',1);word('uni_active',case%2)
 m.call('_universe_reset',[m.symbols['uni_state']]);call('universe_reset',[sy['uni_state']])
 seed=r.randbytes(4);m.u.mem_write(m.symbols['asm_laser']+5,seed);u.mem_write(sy['asm_laser']+5,seed)
 for tick in range(16):
  invoke('asm_encounters')
  for name,n in [('asm_vectors',432),('asm_laser',12),('asm_enc_params',8),('uni_state',30)]:equal(name,n)
  integer('asm_enc_spawns',4)
  for slot in range(12):
   a=int.from_bytes(m.u.mem_read(m.symbols['asm_ships']+slot*272,4),'big');b=int.from_bytes(u.mem_read(sy['asm_ships']+slot*272,4),'little')
   assert int.from_bytes(m.u.mem_read(a+28,2),'big')==int.from_bytes(u.mem_read(b+28,2),'little'),(case,slot,'model kind')
 a=bytes(m.u.mem_read(0x20000,32768));b=bytes(u.mem_read(0x310000,32768));assert a==b,(case,'warp frame');digest.update(a)
print('PASS: 128 original cheat/warp scenarios and 2048 encounter orchestration steps; state, RNG, model kinds and full frames; SHA256',digest.hexdigest())
