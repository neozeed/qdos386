#!/usr/bin/env python3
"""Resume unfinished F2 cases; never alter the retained first-run evidence."""
import hashlib,importlib.util,json,mmap,re,struct,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
D=Path(sys.argv[1]).resolve();S=Path(sys.argv[2]).resolve();B=D.parent
assert str(D).startswith('/tmp/qdos386-') and str(Path(__file__).resolve()).startswith('/tmp/')
spec=importlib.util.spec_from_file_location('decoder',S/'validation/r7/fat32-decode.py');dec=importlib.util.module_from_spec(spec);spec.loader.exec_module(dec)
rows=[];(D/'cases').mkdir(exist_ok=False)
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
def run(cmd):return subprocess.run(list(map(str,cmd)),capture_output=True,text=True,timeout=60)
def copy(src,dst):subprocess.run(['cp','--sparse=always',str(src),str(dst)],check=True)
def save(): (D/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
def geometry(p):
 with p.open('rb') as f:b=f.read(512)
 return dict(spc=b[13],nf=b[16],res=struct.unpack_from('<H',b,14)[0],spf=struct.unpack_from('<I',b,36)[0],total=struct.unpack_from('<I',b,32)[0],ext=struct.unpack_from('<H',b,40)[0],root=struct.unpack_from('<I',b,44)[0])
def diff(before,after,log=None):
 g=geometry(before);changed=[];written=set(int(x) for x in re.findall(r'WRITE \d+ LBA (\d+)',log or ''))
 with before.open('rb') as f,after.open('rb') as h,mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as a,mmap.mmap(h.fileno(),0,access=mmap.ACCESS_READ) as b:
  for i in range(len(a)//512):
   if a[i*512:(i+1)*512]!=b[i*512:(i+1)*512]:changed.append(i)
  active=g['ext']&15;inactive=[]
  if g['ext']&128:
   for n in range(g['nf']):
    if n==active:continue
    off=(g['res']+n*g['spf'])*512
    assert a[off:off+g['spf']*512]==b[off:off+g['spf']*512], 'inactive FAT changed'
    inactive.append(hashlib.sha256(a[off:off+g['spf']*512]).hexdigest())
 if log is not None:assert set(changed)<=written,('unaccounted sectors',set(changed)-written)
 return dict(changed_sectors=changed,inactive_fat_sha256=inactive,all_changed_sectors_in_callback_log=log is not None)
def judge(p,tag,expected=None,compat=False,valid=True):
 info=dec.inspect(p);(D/'logs'/f'{tag}.decode.json').write_text(json.dumps(info,indent=2)+'\n')
 ck=run(['fsck.fat','-n',p]);(D/'logs'/f'{tag}.fsck.txt').write_text(ck.stdout+ck.stderr)
 second=None
 if compat:
  g=geometry(p);assert g['ext']&128
  active=g['ext']&15
  other=run(['fsck.fat','-n','-F',active+1,p]);second=other.returncode
  (D/'logs'/f'{tag}.active.fsck.txt').write_text(other.stdout+other.stderr)
  # Permit only the known selected-FAT synchronization diagnostic, not a general nonzero exit.
  lines=[line for line in other.stdout.splitlines() if line.strip()]
  allowed=[r'fsck.fat 4\.2 .*',r'Using (first|second) FAT\.',r'Free cluster summary uninitialized \(should be \d+\)',r'Leaving filesystem unchanged\.',re.escape(str(p))+r': \d+ files, \d+/\d+ clusters']
  assert not other.stderr and other.returncode in (0,1)
  assert all(any(re.fullmatch(x,line) for x in allowed) for line in lines),other.stdout
 if valid:
  assert not info['errors'],(tag,info['errors'])
  assert ck.returncode==0 or compat,(tag,ck.stdout)
  for name,body in (expected or {}).items():assert info['files'][name]['sha256']==hashlib.sha256(body).hexdigest(),(tag,name)
 return dict(fsck_status=ck.returncode,active_fsck_status=second,decoder_errors=info['errors'],sha256=sha(p),files=info['files'])
def invoke(p,scenario,hint=70010,fail=0,partial=0,ro=0):
 return run([D/'mutation-host','--disposable',p,scenario,hint,fail,partial,ro])
def test(tag,baseline,scenario,hint=70010,expected=None,compat=False,ro=0,malformed=False):
 p=D/'cases'/(tag+'.disposable.img');copy(baseline,p)
 r=invoke(p,scenario,hint,ro=ro);(D/'logs'/f'{tag}.txt').write_text(r.stdout+r.stderr)
 proof=judge(p,tag,expected,compat,valid=not malformed)
 changes=diff(baseline,p,r.stdout)
 row=dict(test=tag,status='PASS' if r.returncode==0 else 'FAIL',image=str(p),api_status=r.returncode,**proof,**changes)
 rows.append(row);save()
 assert r.returncode==0,(tag,r.stdout[-1200:])
 if malformed or ro:assert not changes['changed_sectors']
 print('PASS',tag,flush=True);return p
payload=bytes((i*37+19)&255 for i in range(10000));final=bytearray(payload[:700]+b'\0'*1300);final[123:140]=payload[9000:9017]
flags=['-D_XOPEN_SOURCE=700','-std=c89','-Wall','-Wextra','-Werror','-DQDOS_FS_WRITE','-DQDOS_VFAT','-DQDOS_APOLLO_FAT32','-DQDOS_APOLLO_FAT32_MUTATION','-I'+str(S/'source/kernel')]
subprocess.run(['cc',*flags,str(S/'validation/r7/fat32-mutation.c'),str(S/'source/kernel/fat16.c'),'-o',str(D/'mutation-host')],check=True)
base=Path('/tmp/qdos386-r7-1wn7185l/fat32-fixtures/independent-root7.img')
try:
 # Previously failing result: read-only reclassification, no repeated mutation.
 old=B/'host-matrix/active-fat1.disposable.img';orig=B/'f2-blocker/active-fat1-before.disposable.img'
 proof=judge(old,'existing-active-fat1',{'/HIGH.DAT':payload[:512]},True)
 changes=diff(orig,old,(B/'f2-blocker/replay.txt').read_text());assert changes['changed_sectors']==[1,7,1839,2554,72562]
 rows.append(dict(test='existing-active-fat1',status='PASS_WITH_DOCUMENTED_CHECKER_LIMITATION',**proof,**changes));save()
 test('active-fat1-exercise',old,'exercise',65535,{'/HIGH.DAT':payload[:512],'/PARENT/CHILD/FINAL.DAT':final},True)
 for scenario,hint in [('fragment',70010),('create',65535),('create',65536),('readonly',70010)]:
  ex={'/FRAG.BIN':payload[:1000]+b'\0'*1000} if scenario=='fragment' else {'/HIGH.DAT':payload[:512]} if scenario=='create' else {}
  test(scenario+'-'+str(hint),base,scenario,hint,ex,ro=int(scenario=='readonly'))
 # Larger geometry has valid cluster 131071. Create only a new fixture, not an old completed case.
 big=D/'large-before.disposable.img'
 with big.open('xb') as f:f.truncate(80*1024*1024)
 assert run(['mkfs.fat','--invariant','-F','32','-s','1',big]).returncode==0
 test('boundary-131071',big,'create',131071,{'/HIGH.DAT':payload[:512]})
 test('longname',base,'longname',70010,{'/RENAME~1.TXT':payload[:600]})
 # Full/near-full valid file ownership, no fake lost clusters or bad-cluster shortcut.
 for free in [0,2]:
  p=D/(f'full-{free}-before.disposable.img');copy(big,p);g=geometry(p);data=g['res']+g['nf']*g['spf'];count=(g['total']-data)//g['spc'];last=count+1-free
  with p.open('r+b') as f:
   f.seek(g['res']*512);fat=bytearray(f.read(g['spf']*512))
   for c in range(3,last+1):struct.pack_into('<I',fat,c*4,c+1 if c<last else 0xfffffff)
   for n in range(g['nf']):f.seek((g['res']+n*g['spf'])*512);f.write(fat)
   ent=bytearray(32);ent[:11]=b'HOG     BIN';ent[11]=32;struct.pack_into('<H',ent,26,3);struct.pack_into('<I',ent,28,(last-2)*512)
   f.seek(data*512);f.write(ent)
   for sec in [1,7]:f.seek(sec*512+488);f.write(struct.pack('<II',free,last+1 if free else 0xffffffff))
  judge(p,f'full-{free}-before')
  test(f'full-{free}',p,'full' if not free else 'nearfull',last+1 if free else 2)
 # Bad chain inputs: open must refuse without a sector write; negative fsck results are retained.
 for tag,nx in [('loop',70000),('free',0),('reserved',0xffffff0),('bad',0xffffff7),('outside',200000),('short',0xfffffff)]:
  p=D/(f'malformed-{tag}-before.disposable.img');copy(base,p);g=geometry(p)
  with p.open('r+b') as f:
   for n in range(g['nf']):f.seek((g['res']+n*g['spf'])*512+70000*4);f.write(struct.pack('<I',nx|0xa0000000))
  test('malformed-'+tag,p,'badchain',malformed=True)
except BaseException as e:
 rows.append(dict(status='STOP',error=str(e)));save();raise
print('PASS resumed non-injected F2 cases',len(rows),flush=True)
