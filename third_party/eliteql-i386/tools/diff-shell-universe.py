#!/usr/bin/env python3
"""Original compact universe and fixed-slot spawning differential tests."""
from oracle_pair import *
from unicorn import m68k_const as mr
m=Oracle(root/'oracle/full-shell-m68k.o');m.u.mem_map(0x20000,0x9000)
u.mem_write(sy['elite_screen'],struct.pack('<I',0x310000))
r=random.Random(0x348641);digest=hashlib.sha256()
def invoke(name,d0=0,slot=0):
 m.u.reg_write(mr.UC_M68K_REG_D0,d0);m.u.reg_write(mr.UC_M68K_REG_A4,m.symbols['asm_vectors']+slot*36);m.u.reg_write(mr.UC_M68K_REG_A5,m.symbols['asm_ships']+slot*272)
 m.call(name,[])
 u.reg_write(x.UC_X86_REG_EAX,d0);u.reg_write(x.UC_X86_REG_ESI,sy['asm_vectors']+slot*36);u.reg_write(x.UC_X86_REG_EDI,sy['asm_ships']+slot*272)
 sp=0x2fff00;u.mem_write(sp,struct.pack('<I',0x2ff000));u.reg_write(x.UC_X86_REG_ESP,sp);u.emu_start(sy[name],0x2ff000,count=3000000);assert u.reg_read(x.UC_X86_REG_EIP)==0x2ff000,name

def put(name,data):m.u.mem_write(m.symbols[name],data);u.mem_write(sy[name],data)
def word(name,v):m.u.mem_write(m.symbols[name],struct.pack('>H',v));u.mem_write(sy[name],struct.pack('<H',v))
def equal(name,size):
 a=bytes(m.u.mem_read(m.symbols[name],size));b=bytes(u.mem_read(sy[name],size));assert a==b,(case,name,next((j,aa,bb) for j,(aa,bb) in enumerate(zip(a,b)) if aa!=bb));digest.update(a)
for case in range(128):
 word('asm_mode',3);word('asm_count',12);word('asm_salvage',2);word('uni_active',1);put('flow_config',bytes([255,0,0]));invoke('asm_setup')
 m.call('_universe_reset',[m.symbols['uni_state']]);call('universe_reset',[sy['uni_state']])
 for kind in [2,8,9,8,6]:invoke('uni_allocate',kind);equal('uni_state',30)
 state=bytearray(r.randbytes(432))
 for j in range(12):state[j*36+31]=0
 put('asm_vectors',bytes(state));put('asm_player',bytes([4,255,case%6,0]));word('asm_hidden',1)
 # Removal must compact all blocks and preserve original missile/lock rules.
 slot=case%6;word('asm_slot',slot)
 invoke('uni_remove_current',slot=slot)
 for name,size in [('uni_state',30),('asm_vectors',432),('asm_player',4),('asm_commander',10),('asm_placeholder',36)]:equal(name,size)
 # Child allocation and initialization, including full-pool failure RNG.
 word('asm_spawn_kind',[6,8,9][case%3]);word('uni_active',case%2)
 put('asm_player',bytes([4,0,2,case%2]));put('asm_laser',bytes([0,0,0,0,0])+r.randbytes(4)+bytes(3))
 # A valid parent orientation/position comes from original scene setup.
 invoke('asm_setup');word('uni_active',case%2);word('asm_spawn_kind',[6,8,9][case%3]);put('asm_player',bytes([4,0,2,case%2]))
 if case%4==0:
  word('uni_active',1)
  for j in range(12):invoke('uni_allocate',8)
 invoke('asm_spawn_missile',slot=0)
 for name,size in [('uni_state',30),('asm_vectors',432),('asm_player',4),('asm_laser',12),('asm_commander',10)]:equal(name,size)
 for name in ['asm_launches','asm_debris','asm_pods']:
  assert int.from_bytes(m.u.mem_read(m.symbols[name],4),'big')==int.from_bytes(u.mem_read(sy[name],4),'little'),(case,name)
print('PASS: 128 allocation/removal/missile/cargo/pod scenarios; original state and RNG; SHA256',digest.hexdigest())
