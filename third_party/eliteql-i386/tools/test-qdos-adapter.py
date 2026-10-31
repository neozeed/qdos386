#!/usr/bin/env python3
"""Execute actual GAS 1.38 adapter bytes against recording public trap fixtures."""
from oracle_pair import *
from unicorn import UC_HOOK_INTR
calls=[];events=[];ticks=0;io_result=0

def trap(uc,no,data):
 assert no==48
 fn=uc.reg_read(x.UC_X86_REG_EAX);a=uc.reg_read(x.UC_X86_REG_EBX);b=uc.reg_read(x.UC_X86_REG_ECX);c=uc.reg_read(x.UC_X86_REG_EDX);calls.append((fn,a,b,c));out=0
 if fn==59 and a==32:
  n=min(c,len(events));uc.mem_write(b,struct.pack('<'+'I'*n,*events[:n]) if n else b'');del events[:n];out=n
 elif fn==32:out=ticks
 elif fn==8:
  assert b==0 and c==0, ('suspend must not write a wake flag into game data',a,b,c)
 elif fn==42:out=io_result
 elif fn in [22,26]:out=io_result
 elif fn==59 and a in [6,20]:out=1
 elif fn==16:out=io_result
 elif fn not in [17,18,21,43,8]:raise AssertionError((fn,a,b,c))
 uc.reg_write(x.UC_X86_REG_EAX,out&0xffffffff)
u.hook_add(UC_HOOK_INTR,trap)
p=0x310000;u.mem_write(p,bytes(8))
# Every source-relevant set-1 key maps independently, and releases clear it.
expected={30:(4,4),45:(7,3),31:(3,3),51:(2,2),52:(7,7),53:(7,5),43:(1,5),49:(7,6),59:(0,1),60:(0,3),61:(0,4),62:(0,0),63:(0,5),0x148:(1,2),0x150:(1,7),0x14b:(1,1),0x14d:(1,4),28:(1,0),14:(1,1),1:(1,3)}
for code,(row,bit) in expected.items():
 events.append(code|0x200);u.mem_write(p+6,bytes([row]));assert call('platform_keyrow',[p])==1<<bit,(code,row)
 events.append(code|0x400);assert call('platform_keyrow',[p])==0,code
# Shared physical modifiers do not release each other.
events.extend([42|512,54|512]);u.mem_write(p+6,b'\x07');assert call('platform_keyrow',[p])==1
events.append(42);assert call('platform_keyrow',[p])==1
events.append(54|1024);assert call('platform_keyrow',[p])==0
# Rational 50-Hz conversion retains fractional phase through native tick wrap.
ticks=0xfffffff0;call('platform_poll_install',[sy['asm_clock']]);u.mem_write(sy['asm_ticks'],struct.pack('<I',0xffffff00));start=ticks
for delta in range(1,1001):
 ticks=(start+delta)&0xffffffff;call('platform_poll',[])
 want=(0xffffff00+delta*596600//1193182)&0xffffffff
 assert int.from_bytes(u.mem_read(sy['asm_ticks'],4),'little')==want
# Dirty caller-save registers must never become the OS optional wake pointer.
u.reg_write(x.UC_X86_REG_ECX,sy['ship_fit']);u.reg_write(x.UC_X86_REG_EDX,0x12345678)
call('platform_yield50',[5]);assert (8,10,0,0) in calls
assert call('platform_sound_ipc',[p])==0xffffffed
# Counted original filename -> existing open_ex flags; explicit error propagation.
u.mem_write(p,b'\0\x0cWIN1_JAMESON\0');io_result=7
assert call('platform_file_open',[p,3,p+256])==0
assert calls[-1][0:1]==(42,) and calls[-1][2]==142
assert bytes(u.mem_read(calls[-1][1],13))==b'WIN1_JAMESON\0'
assert int.from_bytes(u.mem_read(p+256,4),'little')==7
io_result=-7;assert call('platform_file_open',[p,1,p+256])==0xfffffff9
for result in [148,100,-10]:
 io_result=result;got=call('platform_file_transfer',[7,p,148,1,p+256]);assert got==(result&0xffffffff if result<0 else 0);assert int.from_bytes(u.mem_read(p+256,4),'little')==max(result,0)
# Exhaustive packed MODE 8 colour decode: all colour bits, all four positions.
for v in range(65536):
 g=bytes(u.mem_read(sy['platform_green']+(v&255)*4,4));rb=bytes(u.mem_read(sy['platform_redblue']+(v>>8)*4,4))
 for j in range(4):assert (g[j]|rb[j])==(((v>>(7-2*j))&1)*4+((v>>(15-2*j))&1)*2+((v>>(14-2*j))&1))
print('PASS: native QDOS adapter key events/releases, 1000 rational-clock/wrap samples, counted filenames, file errors/short transfers, all 65536 packed pixel words; unsupported sound remains -19')
