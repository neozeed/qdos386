#!/usr/bin/env python3
"""Bounded full-machine session, hardware PS/2 input and read-only observation.
Uses unchanged KCTX-FIX2 VGA kernel and a copy of its public volume.
"""
from pathlib import Path
import struct,subprocess,time,json,sys
from PIL import Image
r=Path(__file__).resolve().parent.parent;d=r/'build/os-session'
b=(r/'build/aout/ELITE.AOU').read_bytes();h=struct.unpack_from('<8I',b);o=32+h[1]+h[2];st=b[o+h[4]:];sy={}
for i in range(0,h[4],12):
 ix,t,ot,de,v=struct.unpack_from('<IBBHI',b,o+i);sy[st[ix:].split(b'\0')[0].decode()]=v
(d/'observer.commands').write_text('');log=(d/'emulator.log').open('w');p=subprocess.Popen([str(r/'build/tools/tiny-observer'),'run.ini'],cwd=d,stdout=log,stderr=subprocess.STDOUT)
results=[]
def command(s):
 with (d/'observer.commands').open('a') as f:f.write(s+'\n')
def waitfile(path,size):
 deadline=time.time()+10
 while time.time()<deadline:
  if path.exists() and path.stat().st_size==size:return
  time.sleep(.05)
 raise RuntimeError('observer timeout '+str(path))
def capture(tag):
 raw=d/(tag+'.raw');ram=d/(tag+'-ram.bin');raw.unlink(missing_ok=True);ram.unlink(missing_ok=True)
 command('frame '+raw.name);command('ram '+ram.name+' 0 1000000');waitfile(raw,720*480*4);waitfile(ram,16*1024*1024)
 Image.frombytes('RGB',(720,480),raw.read_bytes(),'raw','BGRX').save(d/(tag+'.png'))
 data=ram.read_bytes();pat=struct.pack('<II',0x45514248,0x36383030);at=0;candidates=[]
 while True:
  at=data.find(pat,at)
  if at<0:break
  offset=at-sy['asm_stats']
  if int.from_bytes(data[offset+sy['asm_ready']:offset+sy['asm_ready']+2],'little')==1:candidates.append(offset)
  at+=1
 result={'tag':tag,'active_copies':len(candidates)}
 if candidates:
  offset=candidates[-1]
  for name in ['asm_ready','asm_mode','asm_docked','asm_page','asm_shop','asm_nav','flow_state','asm_dead','asm_market_result','quit_active','cheat_active']:
   a=offset+sy[name];result[name]=int.from_bytes(data[a:a+2],'little')
  for name in ['asm_ticks','asm_frames','asm_snapshot','platform_last_present']:
   a=offset+sy[name];result[name]=int.from_bytes(data[a:a+4],'little')
  result['ship_fit']=list(data[offset+sy['_ship_fit']:offset+sy['_ship_fit']+8])
  result['cash']=int.from_bytes(data[offset+sy['asm_commander']:offset+sy['asm_commander']+4],'big')
  frame=data[offset+sy['platform_screen']:offset+sy['platform_screen']+32768]
  result['flash_bits']=sum(bool(c&85) for c in frame[::2])
 results.append(result);(d/'session.json').write_text(json.dumps(results,indent=2)+'\n');print(result,flush=True);return result

def typekeys(s,hold=.18):
 subprocess.run([sys.executable,str(r/'tools/tiny-input.py'),s,str(hold)],check=True)
def settle(seconds=1):time.sleep(seconds)
try:
 deadline=time.time()+45
 while time.time()<deadline:
  if '/> ' in (d/'emulator.log').read_text(errors='replace'):break
  if p.poll() is not None:raise RuntimeError('guest stopped during boot')
  time.sleep(.2)
 else:raise RuntimeError('boot prompt timeout')
 typekeys('run elite.aou\n');settle(4);s=capture('01-title');assert s.get('flow_state')==1,s
 typekeys('n',.35);settle(1);s=capture('02-ready');assert s.get('flow_state')==2,s
 typekeys('x',.35);settle(1);s=capture('03-status');assert s.get('flow_state')==0 and s.get('asm_docked')==1,s
 # Use the existing cash cheat to make persistence observable after relaunch.
 typekeys('0',.35);settle();typekeys('1',.35);settle();typekeys('0',.35);settle()
 s=capture('03b-cash');assert s.get('cash')==11000,s
 typekeys('6',.35);settle();s=capture('04-market');assert s.get('asm_page')==0,s
 typekeys('8',.35);settle();s=capture('05-equipment');assert s.get('asm_shop')==1,s
 typekeys('s',.35);settle();s=capture('06-save-name');assert s.get('flow_state')==4,s
 typekeys('\n',.35);settle(2);s=capture('07-saved');assert s.get('asm_market_result')==5,s
 typekeys('8',.35);settle();typekeys('l',.35);settle(4);s=capture('08-flight');assert s.get('asm_docked')==0 and s.get('asm_frames',0)>0,s
 typekeys('a',.5);typekeys('x',.5);settle();capture('09-steering')
 typekeys('\\',.35);settle();s=capture('10-quit');assert s.get('quit_active')==1,s
 typekeys('y',.35);settle(2);s=capture('11-shell');assert s['active_copies']==0,s
 typekeys('run elite.aou\n');settle(2)
 if '--title-load' in sys.argv:
  typekeys('y',.35);settle();capture('12-load-name')
  typekeys('\n',.35);settle(2);s=capture('13-loaded');assert s.get('flow_state')==2,s
 else:
  typekeys('n',.35);settle();typekeys('x',.35);settle();typekeys('6',.35);settle();typekeys('8',.35);settle()
  s=capture('12-equipment');assert s.get('asm_shop')==1 and s.get('cash')==1000,s
  typekeys('o',.35);settle();s=capture('13-load-name');assert s.get('flow_state')==3,s
  typekeys('\n',.35);settle(2);s=capture('14-loaded');assert s.get('flow_state')==0 and s.get('asm_market_result')==6 and s.get('cash')==11000 and s.get('ship_fit')==[22,15,0,0,0,0,0,0],s
 typekeys('\\',.35);settle();typekeys('y',.35);settle(2);typekeys('shutdown\n');p.wait(timeout=15)
 print('PASS: full-machine title/status/market/equipment/save/launch/steer/quit/relaunch/load/shutdown',flush=True)
finally:
 if p.poll() is None:p.terminate();p.wait(timeout=10)
 log.close()
