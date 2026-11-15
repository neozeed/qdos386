#!/usr/bin/env python3
"""Disposable F2 qualification. Run a copy in the task's /tmp workspace."""
import hashlib, importlib.util, json, os, struct, subprocess, sys
from pathlib import Path
sys.dont_write_bytecode=True
B=Path(sys.argv[1]).resolve(); SRC=Path(sys.argv[2]).resolve()
assert str(B).startswith('/tmp/qdos386-') and str(Path(__file__).resolve()).startswith('/tmp/')
D=B/'host-matrix';D.mkdir(exist_ok=False)
spec=importlib.util.spec_from_file_location('decoder',SRC/'validation/r7/fat32-decode.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
rows=[]
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def run(cmd):return subprocess.run(list(map(str,cmd)),capture_output=True,text=True,timeout=60)
def save(): (D/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
def judge(p,tag,expected=None,valid=True):
 ck=run(['fsck.fat','-n',p]);(D/(tag+'.fsck.txt')).write_text(ck.stdout+ck.stderr)
 info=decoder.inspect(p);(D/(tag+'.decode.json')).write_text(json.dumps(info,indent=2)+'\n')
 if valid:
  assert ck.returncode==0,(tag,ck.stdout,ck.stderr)
  assert not info['errors'],(tag,info['errors'])
  for name,body in (expected or {}).items():
   assert info['files'][name]['sha256']==hashlib.sha256(body).hexdigest(),(tag,name)
 return dict(fsck_rc=ck.returncode,decoder_errors=info['errors'],sha256=sha(p))
def invoke(p,scenario,hint=70010,fail=0,partial=0,ro=0):
 result=run([B/'mutation-host','--disposable',p,scenario,hint,fail,partial,ro])
 return result
payload=bytes((i*37+19)&255 for i in range(10000))
final=bytearray(payload[:700]+b'\0'*1300);final[123:140]=payload[9000:9017]
subprocess.run(['cc','-D_XOPEN_SOURCE=700','-std=c89','-Wall','-Wextra','-Werror','-DQDOS_FS_WRITE','-DQDOS_VFAT','-DQDOS_APOLLO_FAT32','-DQDOS_APOLLO_FAT32_MUTATION','-I'+str(SRC/'source/kernel'),str(SRC/'validation/r7/fat32-mutation.c'),str(SRC/'source/kernel/fat16.c'),'-o',str(B/'mutation-host')],check=True)
profiles=[('spc1',80,1,2,False),('spc2',128,2,2,False),('spc8',512,8,2,False),('one-fat',80,1,1,False),('active-fat1',80,1,2,True)]
try:
 for tag,mb,spc,nf,active in profiles:
  p=D/(tag+'.disposable.img')
  with p.open('xb') as f:f.truncate(mb*1024*1024)
  mk=run(['mkfs.fat','--invariant','-F','32','-s',spc,'-f',nf,p]);assert mk.returncode==0
  with p.open('r+b') as f:
   b=bytearray(f.read(512));res=struct.unpack_from('<H',b,14)[0];spf=struct.unpack_from('<I',b,36)[0];data=res+nf*spf
   if active:
    struct.pack_into('<H',b,40,0x81)
    for s in (0,6):f.seek(s*512);f.write(b)
   for copy in range(nf):
    f.seek((res+copy*spf)*512+70010*4);f.write(struct.pack('<I',0xa0000000))
   f.seek((data+(70010-2)*spc)*512);f.write(b'\xa5'*(spc*512))
   f.seek(res*512);inactive_before=hashlib.sha256(f.read(spf*512)).hexdigest()
  judge(p,tag+'-before')
  for scenario,hint,expected in [('create',70010,{'/HIGH.DAT':payload[:512]}),('exercise',65535,{'/HIGH.DAT':payload[:512],'/PARENT/CHILD/FINAL.DAT':final}),('overflow',2,{'/OVER.DAT':b''})]:
   r=invoke(p,scenario,hint);name=tag+'-'+scenario;(D/(name+'.txt')).write_text(r.stdout+r.stderr)
   assert r.returncode==0,(name,r.stdout[-1000:])
   proof=judge(p,name,expected)
   with p.open('rb') as f:
    if active:
     f.seek(res*512);assert hashlib.sha256(f.read(spf*512)).hexdigest()==inactive_before
    copy=1 if active else 0
    f.seek((res+copy*spf)*512+70010*4);assert struct.unpack('<I',f.read(4))[0]==0xafffffff
    if scenario=='create' and spc>1:
     f.seek((data+(70010-2)*spc+1)*512);assert f.read((spc-1)*512)==b'\0'*((spc-1)*512)
   rows.append(dict(test=name,status='PASS',**proof));save();print('PASS',name,flush=True)
 # Exact existing fragmented fixture, and boundary allocations.
 baseline=Path('/tmp/qdos386-r7-1wn7185l/fat32-fixtures/independent-root7.img')
 for scenario,hint in [('fragment',70010),('create',65535),('create',65536),('readonly',70010)]:
  tag=scenario+'-'+str(hint);p=D/(tag+'.disposable.img');subprocess.run(['cp','--sparse=always',baseline,p],check=True)
  before=sha(p);r=invoke(p,scenario,hint,ro=int(scenario=='readonly'));(D/(tag+'.txt')).write_text(r.stdout+r.stderr)
  assert r.returncode==0,(tag,r.stdout[-1000:])
  expect={'/FRAG.BIN':payload[:1000]+b'\0'*1000} if scenario=='fragment' else {'/HIGH.DAT':payload[:512]} if scenario=='create' else {}
  proof=judge(p,tag,expect)
  if scenario=='readonly':assert before==proof['sha256']
  rows.append(dict(test=tag,status='PASS',**proof));save();print('PASS',tag,flush=True)
except BaseException as e:
 rows.append(dict(status='FAIL',error=str(e)));save();raise
print('PASS host successful-operation matrix',len(rows),flush=True)
