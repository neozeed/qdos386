#!/usr/bin/env python3
"""Bounded canonical disk smoke; original image read only, all evidence under /tmp."""
import argparse,hashlib,importlib.util,json,os,struct,subprocess,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
ap=argparse.ArgumentParser();ap.add_argument('--image',type=Path,required=True);ap.add_argument('--work',type=Path,required=True);ap.add_argument('--private',action='store_true');a=ap.parse_args()
R=Path(__file__).resolve().parents[2];B=a.work.resolve();assert str(B).startswith('/tmp/') and not B.exists();B.mkdir();(B/'logs').mkdir();(B/'tmp').mkdir();os.environ['QDOS_R7_WORK']=str(B)
from guest import Guest
image=B/'run.disposable.img';subprocess.run(['cp','--sparse=always',str(a.image.resolve()),str(image)],check=True)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
original=sha(a.image);g=Guest(image,'native');rows=[];failure=None

def command(c,expected,timeout=90):
 start=(g.log/'debug.txt').stat().st_size;screen=g.command(c,timeout);out=(g.log/'debug.txt').read_bytes()[start:].decode(errors='replace');rows.append(dict(command=c,debug=out,screen=screen));assert expected in out,(c,out);print(c,'PASS',flush=True);return out

def held(key):g.hmp('sendkey '+key+' 300');time.sleep(.65)
try:
 assert 'WIN1 writable system, WIN0 read-only FAT16 recovery' in (g.log/'debug.txt').read_text()
 command('RUN F32IO','F32IO PASS')
 command('RUN SH','R13B shell');command('ECHO CANONICAL-SH-PASS','CANONICAL-SH-PASS');command('EXIT','exit status 0')
 command('RUN FDISK --image ide0','LIVE: writes refused')
 if a.private:
  command('RUN GCC -o /TMP/R7B2HI.AOU /HOME/HI.C','exit status 0',150);command('RUN /TMP/R7B2HI.AOU','FIX1 guest GCC works!')
  g.type('RUN BOUNCE 180 2\n');time.sleep(.4);g.key('ctrl-esc');time.sleep(.3);g.key('end');g.key('ret');time.sleep(1);g.hmp('screendump '+str(g.log/'bounce.ppm'));g.key('alt-f1');g.wait(g.prompt,30);command('TYPE /BOUNCE.LOG','verify=PASS')
  command('CD /GAMES/HACK','');g.type('RUN HACK\n');g.wait(lambda s:'experienced player?' in s,15);held('n');g.wait(lambda s:'Hit space to continue' in s,10);held('spc');g.wait(lambda s:'Level 1' in s,15)
  if '--More--' in g.screen():held('spc')
  g.hmp('screendump '+str(g.log/'hack.ppm'));held('shift-q');g.wait(lambda s:'Really quit?' in s,10);held('y');g.wait(g.prompt,15);command('CD /','')
  g.type('START ELITE /new\n');time.sleep(1);g.key('ctrl-esc');g.wait(lambda s:'ELITE.AOU' in s,15);g.key('end');g.key('ret');time.sleep(.5);g.hmp('screendump '+str(g.log/'elite.ppm'))
  for key in ['n','x','backslash','q','y']:held(key)
  g.key('alt-f1');g.key('ctrl-esc');screen=g.wait(lambda s:'Task Manager' in s,10);assert 'ELITE.AOU' not in screen and '7 jobs' in screen,screen;rows.append(dict(elite_exit_task_screen=screen));g.hmp('screendump '+str(g.log/'elite-exited.ppm'));g.key('esc');g.key('alt-f1')
 command('SYNC','SYNC: done')
except BaseException as e:failure=str(e)
finally:g.close();(B/'native.json').write_text(json.dumps(dict(rows=rows,failure=failure),indent=2)+'\n')
assert sha(a.image)==original
if failure:raise RuntimeError(failure)

def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
raw=image.read_bytes();reference=a.image.read_bytes();assert raw[:67584*512]==reference[:67584*512]
judges=[]
for i in (0,1):
 start,count=struct.unpack_from('<II',raw,454+16*i);p=B/('partition%d.disposable.img'%i)
 with p.open('xb') as f:
  for off in range(start*512,(start+count)*512,1048576):
   data=raw[off:min(off+1048576,(start+count)*512)]
   if any(data):f.write(data)
   else:f.seek(len(data),1)
  f.truncate(count*512)
 check=subprocess.run(['fsck.fat','-n',str(p)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(B/('fsck%d.txt'%i)).write_bytes(check.stdout);assert check.returncode==0,check.stdout
 if i==0:
  w=module('walker',R/'tools/r11m-fatwalk.py');errors,lost,nodes=w.walk(w.Vol(p),True);assert not errors and not lost
 else:
  d=module('decoder',R/'validation/r7/fat32-decode.py').inspect(p);assert not d['errors'],d['errors'];assert d['files']['/OWNEXIT.DAT']['sha256']==hashlib.sha256(b'\x6d'*700).hexdigest()
  if a.private:assert d['files']['/TMP/R7B2HI.AOU']['sha256']=='5f2973ad508006aeefe79e3e4c0f0c5675e3cb4c25d773fc4d372c3c85a7820f'
  (B/'decoder.json').write_text(json.dumps(d,indent=2)+'\n')
 judges.append(dict(partition=i,fsck=check.returncode,sha256=sha(p),independent_clean=True))
(B/'result.json').write_text(json.dumps(dict(status='PASS',original_sha256=original,mutated_sha256=sha(image),fat16_unchanged=True,private=a.private,judges=judges),indent=2)+'\n');print('PASS native smoke and independent on-disk checks')
