#!/usr/bin/env python3
"""Independent complete original menu/chart/status frame comparisons."""
from oracle_pair import *
m=Oracle(root/'oracle/full-shell-m68k.o');m.u.mem_map(0x20000,0x9000)
u.mem_write(sy['elite_screen'],struct.pack('<I',0x310000));digest=hashlib.sha256()
def word(name,v):m.u.mem_write(m.symbols[name],struct.pack('>H',v));u.mem_write(sy[name],struct.pack('<H',v))
def invoke(name):
 m.call(name,[]);sp=0x2fff00;u.mem_write(sp,struct.pack('<I',0x2ff000));u.reg_write(x.UC_X86_REG_ESP,sp);
 try:u.emu_start(sy[name],0x2ff000,count=3000000)
 except Exception:
  pc=u.reg_read(x.UC_X86_REG_EIP);print(case,name,hex(pc),sorted((v,k) for k,v in sy.items() if v<=pc)[-3:]);raise
 assert u.reg_read(x.UC_X86_REG_EIP)==0x2ff000,name
 a=bytes(m.u.mem_read(0x20000,32768));b=bytes(u.mem_read(0x310000,32768));assert a==b,(case,name,next((j,aa,bb) for j,(aa,bb) in enumerate(zip(a,b)) if aa!=bb));digest.update(a)
word('asm_mode',3);word('asm_salvage',2);word('asm_count',12)
for case in range(16):
 invoke('asm_setup');word('asm_page',case%4);word('asm_market_result',case%4);word('asm_market_selection',case);word('asm_shop_view',case%4)
 for name in ['asm_commander_page','asm_market_page','asm_shop_page','presentation_system','presentation_local','cheat_paint','market_quantity_paint']:
  invoke('asm_clear_view');invoke(name)
print('PASS: 112 complete original status/cargo/market/shop/system/local-chart/cheat/quantity pages, byte-exact full frames; SHA256',digest.hexdigest())
