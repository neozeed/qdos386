#!/usr/bin/env python3
"""Bounded disposable-image tests. Run from an isolated working copy only."""
from pathlib import Path
import argparse,subprocess,struct,json,hashlib,os
ap=argparse.ArgumentParser();ap.add_argument('output',type=Path);a=ap.parse_args();out=a.output.resolve()
assert str(out).startswith('/tmp/') and not out.exists();out.mkdir()
root=Path(__file__).resolve().parents[2];assert str(root).startswith('/tmp/')
bin=root/'source/apps/disktools'; rows=[]
def run(name,tool,p,args=(),stdin='',expected=0,write=False,env=None):
 before=hashlib.sha256(p.read_bytes()).hexdigest();cmd=[str(bin/tool),'--image',str(p),*args]
 q=subprocess.run(cmd,input=stdin,text=True,capture_output=True,timeout=30,env=env)
 (out/(name+'.log')).write_text(q.stdout+q.stderr)
 after=hashlib.sha256(p.read_bytes()).hexdigest();rows.append(dict(name=name,command=cmd,exit=q.returncode,expected=expected,before=before,after=after,no_write=not write))
 (out/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
 assert q.returncode==expected,(name,q.returncode,q.stdout,q.stderr)
 if not write:assert before==after,name
 return q.stdout
p=out/'clean.r7-disposable.img'
with p.open('xb') as f:f.truncate(198656*512)
run('blank','fdisk-host',p)
run('cancel','fdisk-host',p,['--edit','--test-disposable'], 'create 1 2048 65536 16\nquit\n')
run('overflow-overlap','fdisk-host',p,['--edit','--test-disposable'],'create 1 4294967295 65536 16\ncreate 1 2048 65536 16\ncreate 2 4096 131072 32\nquit\n')
run('partition','fdisk-host',p,['--edit','--test-disposable'],'create 1 2048 65536 16\ncreate 2 67584 131072 32\nactive 1\nwrite\n',write=True)
for n,bits in [(1,16),(2,32)]:
 run('format'+str(bits),'format-host',p,['--partition',str(n),'--fat',str(bits),'--test-disposable'],write=True)
 run('scan'+str(bits),'chkdsk-host',p,['--partition',str(n)])
data=p.read_bytes()
def independent(name,data,start,count):
 v=out/(name+'.partition');v.write_bytes(data[start*512:(start+count)*512]);q=subprocess.run(['fsck.fat','-n',str(v)],capture_output=True,text=True,timeout=30);(out/(name+'.fsck')).write_text(q.stdout+q.stderr);assert q.returncode==0,(name,q.stdout)
independent('fat16',data,2048,65536);independent('fat32',data,67584,131072)
def fixture(name,mutate):
 x=bytearray(data);mutate(x);p=out/(name+'.r7-disposable.img');p.write_bytes(x);return p
u16=lambda x,o:struct.unpack_from('<H',x,o)[0]
u32=lambda x,o:struct.unpack_from('<I',x,o)[0]
put16=lambda x,o,v:struct.pack_into('<H',x,o,v)
put32=lambda x,o,v:struct.pack_into('<I',x,o,v)
s=67584*512;res=u16(data,s+14);spf=u32(data,s+36);fat=s+res*512;rootsec=res+2*spf;rd=s+rootsec*512
for answer in ['N','Q','','Y','A']:
 pbad=fixture('info-'+(answer or 'eof'),lambda x:put32(x,s+512+488,0))
 run('info-ro-'+(answer or 'eof'),'chkdsk-host',pbad,['--partition','2'],expected=1)
 result=run('info-repair-'+(answer or 'eof'),'chkdsk-host',pbad,['--partition','2','--repair','--test-disposable'],answer+'\n' if answer else '',expected=2 if answer in ['Y','A'] else 1,write=answer in ['Y','A'])
 assert 'Fix (Y/N/A/Q)?' in result
 if answer in ['Y','A']:run('repaired-'+answer,'chkdsk-host',pbad,['--partition','2']);independent('repaired-'+answer,pbad.read_bytes(),67584,131072)
# A blank line is distinct from EOF and also defaults to No.
pbad=fixture('default-no',lambda x:put32(x,s+512+488,0));run('default-no','chkdsk-host',pbad,['--partition','2','--repair','--test-disposable'],'\n',expected=1)
for name,fn in [('duplicate-active',lambda x:x.__setitem__(462,128)),('gpt',lambda x:x.__setitem__(450,238)),('extended',lambda x:x.__setitem__(450,15)),('bounds',lambda x:put32(x,458,0xffffffff)),('sector-size',lambda x:put16(x,s+11,4096))]:
 pbad=fixture(name,fn);run(name,'chkdsk-host',pbad,['--partition','2'],expected=3)
def entry(x,slot,name,first,size):
 o=rd+slot*32;x[o:o+11]=name;x[o+11]=32;put16(x,o+26,first&65535);put16(x,o+20,first>>16);put32(x,o+28,size)
def setfat(x,c,n):
 for f in range(2):put32(x,fat+f*spf*512+c*4,n)
def corrupt(kind):
 def f(x):
  if kind=='fat-copy':put32(x,fat+20,0x0fffffff)
  elif kind=='orphan':setfat(x,3,0x0fffffff)
  elif kind=='cycle':entry(x,0,b'CYCLE   DAT',3,1024);setfat(x,3,4);setfat(x,4,3)
  elif kind=='crosslink':entry(x,0,b'ONE     DAT',3,512);entry(x,1,b'TWO     DAT',3,512);setfat(x,3,0x0fffffff)
  elif kind=='short-chain':entry(x,0,b'SHORT   DAT',3,1024);setfat(x,3,0x0fffffff)
  elif kind=='bad-first':entry(x,0,b'BAD     DAT',0x0fffffff,0)
  elif kind=='lfn':x[rd]=0x41;x[rd+11]=15;x[rd+12]=1
  elif kind=='bad-dot':
   entry(x,0,b'DIR        ',3,0);x[rd+11]=16;setfat(x,3,0x0fffffff)
 return f
for name in ['fat-copy','orphan','cycle','crosslink','short-chain','bad-first','lfn','bad-dot']:
 pbad=fixture(name,corrupt(name));run(name,'chkdsk-host',pbad,['--partition','2'],expected=1)
 run(name+'-repair-refusal','chkdsk-host',pbad,['--partition','2','--repair','--test-disposable'],'A\n',expected=1)
pbad=fixture('partial',lambda x:None)
run('partial-io','format-host',pbad,['--partition','1','--fat','16','--test-disposable'],expected=3,write=True,env=dict(os.environ,QDOS_DISK_TEST_FAIL_AFTER='2'))
short=out/'truncated.r7-disposable.img';short.write_bytes(data[:1024]);run('truncated','chkdsk-host',short,expected=3)
print('PASS',len(rows),'bounded host invocations plus independent fsck')
