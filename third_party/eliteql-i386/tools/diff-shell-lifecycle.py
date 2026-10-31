#!/usr/bin/env python3
"""Compare complete original shell lifetime paths, not replacement fixtures."""
from oracle_pair import *
from unicorn import m68k_const as mr
m=Oracle(root/'oracle/full-shell-m68k.o');m.u.mem_map(0x20000,0x9000)
def invoke(name):
 m.call(name,[])
 sp=0x2fff00;u.mem_write(sp,struct.pack('<I',0x2ff000));u.reg_write(x.UC_X86_REG_ESP,sp)
 try:u.emu_start(sy[name],0x2ff000,count=3000000)
 except Exception:
  pc=u.reg_read(x.UC_X86_REG_EIP);print(name,hex(pc),sorted((v,k) for k,v in sy.items() if v<=pc)[-5:]);raise
 assert u.reg_read(x.UC_X86_REG_EIP)==0x2ff000,name
 a=bytes(m.u.mem_read(0x20000,32768));b=bytes(u.mem_read(0x310000,32768));assert a==b,(case,name,'frame',next((j,aa,bb) for j,(aa,bb) in enumerate(zip(a,b)) if aa!=bb));digest.update(a)

def put(name,data):
 m.u.mem_write(m.symbols[name],data);u.mem_write(sy[name],data)
def word(name,v):
 m.u.mem_write(m.symbols[name],struct.pack('>H',v));u.mem_write(sy[name],struct.pack('<H',v))
def equal(name,size):
 a=bytes(m.u.mem_read(m.symbols[name],size));b=bytes(u.mem_read(sy[name.lstrip('_')],size))
 assert a==b,(case,name,next((j,aa,bb) for j,(aa,bb) in enumerate(zip(a,b)) if aa!=bb));digest.update(a)
r=random.Random(0x138386);digest=hashlib.sha256()
for case in range(64):
 u.mem_write(sy['elite_screen'],struct.pack('<I',0x310000))
 word('asm_mode',3);word('asm_count',12);word('asm_salvage',case%4)
 put('flow_config',bytes([255,0,0]));invoke('asm_setup')
 # Snapshot and restore byte-serialized commander state.
 invoke('flow_remember')
 equal('flow_saved',148)
 put('asm_cargo',r.randbytes(17));invoke('flow_restore')
 for name,size in [('_ship_fit',8),('asm_commander',10),('asm_cargo',22),('asm_travel',64),('asm_market',34),('asm_player',4)]:equal(name,size)
 invoke('flow_title_load')
 for name,size in [('asm_vectors',432),('asm_clouds',5376)]:equal(name,size)
 invoke('flow_death')
 for name,size in [('asm_vectors',432),('asm_params',4),('asm_controls',13),('asm_keys',6),('asm_laser',12),('asm_dust',168)]:equal(name,size)
 for name in ['flow_state','flow_death_count','flow_index','flow_random_carry','flow_death_flags']:
  assert int.from_bytes(m.u.mem_read(m.symbols[name],2),'big')==int.from_bytes(u.mem_read(sy[name],2),'little'),(case,name)
print('PASS: 64 complete original remember/restore/title/death initialization sequences; SHA256',digest.hexdigest())
