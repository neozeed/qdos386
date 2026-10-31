#!/usr/bin/env python3
"""Original complete shell save/load orchestration, using recorded I/O results.
The separately frozen R2 platform-check covers _commander_io transfer semantics.
"""
from oracle_pair import *
from unicorn import UC_HOOK_CODE,m68k_const as mr
m=Oracle(root/'oracle/full-shell-m68k.o');m.u.mem_map(0x20000,0x9000)
u.mem_write(sy['elite_screen'],struct.pack('<I',0x310000))
r=random.Random(0x45514132);digest=hashlib.sha256();disk={};error=0

def mh(uc,pc,size,data):
 sp=uc.reg_read(mr.UC_M68K_REG_A7);ret,p,write=struct.unpack('>III',bytes(uc.mem_read(sp,12)))
 if not error:
  if write:disk['m']=bytes(uc.mem_read(p,148))
  else:uc.mem_write(p,disk['m'])
 uc.reg_write(mr.UC_M68K_REG_D0,error&0xffffffff);uc.reg_write(mr.UC_M68K_REG_A7,sp+4);uc.reg_write(mr.UC_M68K_REG_PC,ret)
def nh(uc,pc,size,data):
 sp=uc.reg_read(x.UC_X86_REG_ESP);ret,p,write=struct.unpack('<III',bytes(uc.mem_read(sp,12)))
 if not error:
  if write:disk['n']=bytes(uc.mem_read(p,148))
  else:uc.mem_write(p,disk['n'])
 uc.reg_write(x.UC_X86_REG_EAX,error&0xffffffff);uc.reg_write(x.UC_X86_REG_ESP,sp+4);uc.reg_write(x.UC_X86_REG_EIP,ret)
m.u.hook_add(UC_HOOK_CODE,mh,begin=m.symbols['_commander_io'],end=m.symbols['_commander_io']);u.hook_add(UC_HOOK_CODE,nh,begin=sy['commander_io'],end=sy['commander_io'])
def invoke(name):
 try:m.call(name,[])
 except Exception:
  pc=m.u.reg_read(mr.UC_M68K_REG_PC);print(name,hex(pc), sorted((v,k) for k,v in m.symbols.items() if v<=pc)[-4:]);raise
 sp=0x2fff00;u.mem_write(sp,struct.pack('<I',0x2ff000));u.reg_write(x.UC_X86_REG_ESP,sp);u.emu_start(sy[name],0x2ff000,count=3000000);assert u.reg_read(x.UC_X86_REG_EIP)==0x2ff000,name

def put(name,data):m.u.mem_write(m.symbols[name],data);u.mem_write(sy[name],data)
def word(name,v):m.u.mem_write(m.symbols[name],struct.pack('>H',v));u.mem_write(sy[name],struct.pack('<H',v))
def equal(name,size):
 a=bytes(m.u.mem_read(m.symbols[name],size));b=bytes(u.mem_read(sy[name],size));assert a==b,(case,name,next((j,aa,bb) for j,(aa,bb) in enumerate(zip(a,b)) if aa!=bb));digest.update(a)
for case in range(64):
 word('asm_mode',3);word('asm_count',12);word('asm_salvage',2);put('flow_config',bytes([255,0,0]));invoke('asm_setup');invoke('release_commander')
 cmd=bytearray(r.randbytes(10));cmd[7:9]=b'\0\0';put('asm_commander',bytes(cmd));put('asm_cargo',bytes(r.randrange(10) for _ in range(17)))
 word('asm_shop',1);error=0;invoke('flow_save_actual');assert disk['m']==disk['n'],(case,'save bytes');digest.update(disk['m'])
 equal('save_buffer',148);equal('flow_saved',148)
 # Valid EQA2, checksum corruption, invalid semantic payload, I/O error.
 if case%4==1:
  for k in disk:disk[k]=disk[k][:4]+bytes([disk[k][4]^1])+disk[k][5:]
 if case%4==2:
  for k in disk:
   b=bytearray(disk[k]);b[138]=5;v=0
   for c in b[8:]:v=(((v<<1)|(v>>31))+c)&0xffffffff
   b[4:8]=v.to_bytes(4,'big');disk[k]=bytes(b)
 if case%4==3:error=-10
 invoke('flow_load_actual')
 for name,size in [('asm_commander',10),('asm_cargo',22),('asm_travel',64),('asm_player',4),('save_buffer',148),('flow_saved',148)]:equal(name,size)
 assert int.from_bytes(m.u.mem_read(m.symbols['asm_market_result'],2),'big')==int.from_bytes(u.mem_read(sy['asm_market_result'],2),'little'),(case,'load result')
 a=bytes(m.u.mem_read(0x20000,32768));b=bytes(u.mem_read(0x310000,32768));assert a==b,(case,'frame');digest.update(a)
print('PASS: 64 original full save/load sequences, serialized bytes, checksum/validation failures, I/O errors and rendered result; SHA256',digest.hexdigest())

# Actual accepted filename path, including original formatter and modal return.
word('asm_shop',1);word('flow_state',4);word('flow_name_len',7)
put('flow_edit',b'JAMESON\0');error=0
m.u.hook_add(UC_HOOK_CODE,lambda uc,*a:uc.emu_stop(),begin=m.symbols['asm_wait'],end=m.symbols['asm_wait'])
u.hook_add(UC_HOOK_CODE,lambda uc,*a:uc.emu_stop(),begin=sy['asm_wait'],end=sy['asm_wait'])
sp=0x2fff00;m.u.mem_write(sp,struct.pack('>I',0x1ff000));m.u.reg_write(mr.UC_M68K_REG_A7,sp);m.u.emu_start(m.symbols['flow_name_accept'],0x1ff000,count=3000000)
u.mem_write(sp,struct.pack('<I',0x2ff000));u.reg_write(x.UC_X86_REG_ESP,sp);u.emu_start(sy['flow_name_accept'],0x2ff000,count=3000000)
assert m.u.reg_read(mr.UC_M68K_REG_PC)==m.symbols['asm_wait']
assert u.reg_read(x.UC_X86_REG_EIP)==sy['asm_wait']
assert bytes(m.u.mem_read(m.symbols['_commander_filename'],14))==bytes(u.mem_read(sy['commander_filename'],14))==b'\0\x0cWIN1_JAMESON'
assert disk['m']==disk['n']
print('PASS: accepted named-save path, filename and save bytes')

# Preserve and expose an original-source defect: load returns result 6 into a
# four-entry market message table when asm_shop=0 (including title loading).
# Stop at the invalid text pointer, rather than masking it with a fake success.
word('asm_shop',0);word('flow_state',3);word('flow_from_title',1)
observed={}
def bad_m(uc,pc,size,data):
 p=uc.reg_read(mr.UC_M68K_REG_A4)
 if p>0x1000000:observed['m']=p;uc.emu_stop()
def bad_n(uc,pc,size,data):
 p=uc.reg_read(x.UC_X86_REG_ESI)
 if p>0x1000000:observed['n']=p;uc.emu_stop()
m.u.hook_add(UC_HOOK_CODE,bad_m,begin=m.symbols['asm_text'],end=m.symbols['asm_text'])
u.hook_add(UC_HOOK_CODE,bad_n,begin=sy['asm_text'],end=sy['asm_text'])
sp=0x2fff00;m.u.mem_write(sp,struct.pack('>I',0x1ff000));m.u.reg_write(mr.UC_M68K_REG_A7,sp);m.u.emu_start(m.symbols['flow_name_accept'],0x1ff000,count=3000000)
u.mem_write(sp,struct.pack('<I',0x2ff000));u.reg_write(x.UC_X86_REG_ESP,sp);u.emu_start(sy['flow_name_accept'],0x2ff000,count=3000000)
assert observed=={'m':0x2053544f,'n':0x4f545320},observed
assert int.from_bytes(m.u.mem_read(m.symbols['asm_market_result'],2),'big')==int.from_bytes(u.mem_read(sy['asm_market_result'],2),'little')==6
print('CONFIRMED ORIGINAL DEFECT: valid title load indexes asm_market_messages[6], outside four pointers; original A4=0x2053544f, native ESI=0x4f545320 (ASCII used as a pointer). No source behavior patched.')
