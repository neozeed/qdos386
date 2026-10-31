#!/usr/bin/env python3
"""Drive actual asm_loop on both CPUs, with only OS input/time mocked."""
from oracle_pair import *
from unicorn import UC_HOOK_CODE,m68k_const as mr
m=Oracle(root/'oracle/full-shell-m68k.o');m.u.mem_map(0x20000,0x9000)
u.mem_write(sy['elite_screen'],struct.pack('<I',0x310000))
rows=[0]*8;frames=0;digest=hashlib.sha256()
def ret68(value=0):
 m.u.reg_write(mr.UC_M68K_REG_D0,value&0xffffffff)
 sp=m.u.reg_read(mr.UC_M68K_REG_A7);pc=int.from_bytes(m.u.mem_read(sp,4),'big');m.u.reg_write(mr.UC_M68K_REG_A7,sp+4);m.u.reg_write(mr.UC_M68K_REG_PC,pc)
def ret86(value=0):
 u.reg_write(x.UC_X86_REG_EAX,value&0xffffffff)
 sp=u.reg_read(x.UC_X86_REG_ESP);pc=int.from_bytes(u.mem_read(sp,4),'little');u.reg_write(x.UC_X86_REG_ESP,sp+4);u.reg_write(x.UC_X86_REG_EIP,pc)
def mh(uc,pc,size,data):
 if pc==m.symbols['asm_keyrow']:
  row=uc.reg_read(mr.UC_M68K_REG_D0)&7;uc.mem_write(m.symbols['asm_ipc']+6,bytes([row]));ret68(rows[row])
 elif pc==m.symbols['_keyboard_drain']:ret68(0)
 elif pc==m.symbols['asm_wait']:uc.emu_stop()
 elif bytes(uc.mem_read(pc,2))==b'\x4e\x41':
  if uc.reg_read(mr.UC_M68K_REG_D0)==17:
   uc.reg_write(mr.UC_M68K_REG_D0,0xffffffed);uc.reg_write(mr.UC_M68K_REG_PC,pc+2);return
  assert uc.reg_read(mr.UC_M68K_REG_D0)==8,('unexpected trap',hex(pc))
  p=m.symbols['asm_ticks'];t=int.from_bytes(uc.mem_read(p,4),'big');uc.mem_write(p,((t+uc.reg_read(mr.UC_M68K_REG_D3))&0xffffffff).to_bytes(4,'big'));uc.reg_write(mr.UC_M68K_REG_D0,0);uc.reg_write(mr.UC_M68K_REG_PC,pc+2)
def nh(uc,pc,size,data):
 if pc==sy['platform_keyrow']:
  sp=uc.reg_read(x.UC_X86_REG_ESP);p=int.from_bytes(uc.mem_read(sp+4,4),'little');ret86(rows[uc.mem_read(p+6,1)[0]&7])
 elif pc==sy['platform_keyboard_queue_pop']:ret86(-1)
 elif pc==sy['platform_sound_ipc']:ret86(-19)
 elif pc==sy['asm_wait']:uc.emu_stop()
 elif pc==sy['platform_yield50']:
  sp=uc.reg_read(x.UC_X86_REG_ESP);ticks=int.from_bytes(uc.mem_read(sp+4,4),'little');p=sy['asm_ticks'];t=int.from_bytes(uc.mem_read(p,4),'little');uc.mem_write(p,((t+ticks)&0xffffffff).to_bytes(4,'little'));ret86(0)
mb=(root/'oracle/full-shell-m68k.o').read_bytes();mt=struct.unpack_from('>8I',mb)[1];traps=[0x100000+j for j in range(0,mt,2) if mb[32+j:34+j]==b'\x4e\x41']
for pc in [m.symbols[n] for n in ['asm_keyrow','_keyboard_drain','asm_wait']]+traps:m.u.hook_add(UC_HOOK_CODE,mh,begin=pc,end=pc)
for pc in [sy[n] for n in ['platform_keyrow','platform_keyboard_queue_pop','platform_yield50','platform_sound_ipc','asm_wait']]:u.hook_add(UC_HOOK_CODE,nh,begin=pc,end=pc)
def ncall(name):
 sp=0x2fff00;u.mem_write(sp,struct.pack('<I',0x2ff000));u.reg_write(x.UC_X86_REG_ESP,sp);u.emu_start(sy[name],0x2ff000,count=3000000);assert u.reg_read(x.UC_X86_REG_EIP)==0x2ff000

def value(name,v,n=2):
 m.u.mem_write(m.symbols[name],v.to_bytes(n,'big'));u.mem_write(sy[name],v.to_bytes(n,'little'))
def equal(name,size):
 a=bytes(m.u.mem_read(m.symbols[name],size));b=bytes(u.mem_read(sy[name.lstrip('_')],size))
 assert a==b,(frames,name,next((j,aa,bb) for j,(aa,bb) in enumerate(zip(a,b)) if aa!=bb));digest.update(a)
value('asm_mode',3);value('asm_salvage',2);value('asm_count',12)
for uc,ss in [(m.u,m.symbols),(u,sy)]:uc.mem_write(ss['flow_config'],bytes([0,0,0]))
m.call('asm_setup',[]);ncall('asm_setup')
m.call('release_commander',[]);ncall('release_commander')
m.call('flow_remember',[]);ncall('flow_remember')
m.call('flow_title_load',[]);ncall('flow_title_load')
for frames in range(800):
 value('asm_ticks',frames*5,4)
 rows[:]=[0]*8
 if frames==620:rows[1]=32
 if frames==630:rows[7]=64
 if frames==700:value('asm_dead',1)
 if frames==730:rows[1]=64
 if frames==740:rows[7]=64
 if frames==750:rows[7]=8
 # Any fresh key at ready title; Y/N prompt is handled by original dispatch.
 if 320<=frames<336:rows[4]=16
 if 340<=frames<348:rows[7]=8
 if 352<=frames<360:rows[2]=4
 if frames==2800:rows[6]=4
 if frames==416:rows[6]=4
 if frames==264:rows[6]=4
 if frames==280:rows[4]=1
 if frames in [128,256]:rows[7]=8  # X
 try:
  m.u.emu_start(m.symbols['asm_loop'],0x1ff000,count=3000000)
  u.emu_start(sy['asm_loop'],0x2ff000,count=3000000)
 except Exception:
  pc=u.reg_read(x.UC_X86_REG_EIP);print('native',hex(pc),sorted((v,k) for k,v in sy.items() if v<=pc)[-3:]);raise
 assert m.u.reg_read(mr.UC_M68K_REG_PC)==m.symbols['asm_wait']
 assert u.reg_read(x.UC_X86_REG_EIP)==sy['asm_wait']
 for name,size in [('_ship_fit',8),('asm_vectors',432),('asm_controls',13),('asm_params',4),('asm_status',5),('asm_commander',10),('asm_travel',64),('asm_market',34),('asm_player',4),('asm_laser',12),('asm_dust',168)]:equal(name,size)
 for slot in range(12):
  ma=m.symbols['asm_ships']+272*slot;na=sy['asm_ships']+272*slot
  mp=int.from_bytes(m.u.mem_read(ma,4),'big');np=int.from_bytes(u.mem_read(na,4),'little')
  assert bool(mp)==bool(np),(frames,'model presence',slot)
  if mp:assert int.from_bytes(m.u.mem_read(mp+28,2),'big')==int.from_bytes(u.mem_read(np+28,2),'little'),(frames,'model type',slot)
  assert struct.unpack('>6h',bytes(m.u.mem_read(ma+4,12)))==struct.unpack('<6h',bytes(u.mem_read(na+4,12))),(frames,'cache',slot)
  assert bytes(m.u.mem_read(ma+16,256))==bytes(u.mem_read(na+16,256)),(frames,'cache lines',slot)
 a=bytes(m.u.mem_read(0x20000,32768));b=bytes(u.mem_read(0x310000,32768));assert a==b,(frames,'frame',next((j,aa,bb) for j,(aa,bb) in enumerate(zip(a,b)) if aa!=bb));digest.update(a)
 for name in ['flow_state','asm_docked','asm_page','asm_nav','asm_dead']:
  assert int.from_bytes(m.u.mem_read(m.symbols[name],2),'big')==int.from_bytes(u.mem_read(sy[name],2),'little'),(frames,name)
assert int.from_bytes(u.mem_read(sy['asm_frames'],4),'little')>400
assert int.from_bytes(u.mem_read(sy['flow_state'],2),'little')==0 and int.from_bytes(u.mem_read(sy['asm_docked'],2),'little')==1
print('PASS: 800 actual original loop iterations: both complete title approaches, dock, launch, flight, firing, pitch/roll, status overlay, death animation and restart; byte-exact state and full frames; SHA256',digest.hexdigest())
