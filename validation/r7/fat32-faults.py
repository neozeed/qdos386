#!/usr/bin/env python3
"""Every write boundary: reject and latch errors; independently retain damage.
No atomicity claim. Exact output must equal only the successful callback prefix
plus the injected partial sector, with no later write and no unrelated changes.
"""
import hashlib,importlib.util,json,mmap,os,re,struct,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
D=Path(sys.argv[1]).resolve();S=Path(sys.argv[2]).resolve()
assert str(D).startswith('/tmp/') and str(Path(__file__).resolve()).startswith('/tmp/')
E=D/'faults';E.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('decoder',S/'validation/r7/fat32-decode.py');dec=importlib.util.module_from_spec(spec);spec.loader.exec_module(dec)
base=Path('/tmp/qdos386-r7-1wn7185l/fat32-fixtures/independent-root7.img');rows=json.loads((E/'results.json').read_text()) if (E/'results.json').exists() else []; rows=[r for r in rows if r.get('status')!='STOP']; done={r['test'] for r in rows if r.get('status')=='PASS_ERROR_PROPAGATION_AND_EXACT_PREFIX'}
def save(): (E/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
def copy(p):subprocess.run(['cp','--sparse=always',str(base),str(p)],check=True)
def call(p,case,fail=0,part=0,env=None):
 return subprocess.run([str(D/'mutation-fault'),'--disposable',str(p),case,'70010',str(fail),str(part),'0'],capture_output=True,text=True,env=env,timeout=30)
try:
 for case in ['create','exercise','longname']:
  ref=E/(case+'-gold.disposable.img');trace=E/(case+'.trace')
  if not trace.exists():
   copy(ref);env=dict(os.environ,QDOS_F2_TRACE=str(trace))
   r=call(ref,case,env=env);(E/(case+'-gold.txt')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stdout
   check=subprocess.run(['fsck.fat','-n',str(ref)],capture_output=True,text=True);assert check.returncode==0,check.stdout
   assert not dec.inspect(ref)['errors']
  raw=trace.read_bytes();assert len(raw)%516==0
  records=[(struct.unpack_from('<I',raw,i)[0],raw[i+4:i+516]) for i in range(0,len(raw),516)]
  # 0 bytes and a torn sector at every boundary; 512 bytes with failure also
  # tests the ambiguous "device completed the write, then reported error" case.
  for prefix in [0,256,512]:
   expected={}
   for at,(lba,data) in enumerate(records,1):
    tag=f'{case}-{at:04d}-{prefix}';p=E/(tag+'.disposable.img')
    if tag in done:
     expected[lba]=data;continue
    if p.exists():
     # Recover the completed writer invocation after a host-only judge interruption.
     r=subprocess.CompletedProcess([],1,(E/(tag+'.txt')).read_text(),'')
    else:
     copy(p);r=call(p,case,at,prefix);(E/(tag+'.txt')).write_text(r.stdout+r.stderr)
    assert r.returncode==1 and 'RESULT rc=-85' in r.stdout,(tag,r.stdout[-1000:])
    assert 'LATCH flush=-85 state=-2' in r.stdout and 'LATCH next=-85 further-writes=0' in r.stdout,(tag,r.stdout[-1000:])
    assert int(re.findall(r'RESULT rc=-85 writes=(\d+)',r.stdout)[0])==at
    with base.open('rb') as f,p.open('rb') as g:
     b=dec.DiskBytes(f);out=dec.DiskBytes(g)
     def equal_range(start,end):
      for pos in range(start,end,1048576):
       stop=min(end,pos+1048576)
       if out[pos:stop]!=b[pos:stop]:return False
      return True
     expected_here=dict(expected);old=expected_here.get(lba,b[lba*512:(lba+1)*512]);expected_here[lba]=data[:prefix]+old[prefix:]
     # Compare large unchanged spans and the individually explained changed sectors.
     previous=0
     for s,body in sorted(expected_here.items()):
      assert equal_range(previous,s*512),(tag,'unrelated range',previous,s*512)
      assert out[s*512:(s+1)*512]==body,(tag,'wrong partial sector',s)
      previous=(s+1)*512
     assert equal_range(previous,len(out)),(tag,'unrelated tail')
     h=hashlib.sha256()
     for block in iter(lambda:g.read(1048576),b''):h.update(block)
     digest=h.hexdigest()
    ck=subprocess.run(['fsck.fat','-n',str(p)],capture_output=True,text=True,timeout=20)
    (E/(tag+'.strengthened.fsck.txt')).write_text(ck.stdout+ck.stderr);info=dec.inspect(p)
    (E/(tag+'.strengthened.decode.json')).write_text(json.dumps(info,indent=2)+'\n')
    # A clean decoder must also have a clean Linux check. Non-journaled damage
    # is explicitly reported, never normalized into a clean-filesystem claim.
    if not info['errors']:assert ck.returncode==0,(tag,ck.stdout)
    assert ck.returncode in (0,1), (tag,ck.returncode,ck.stdout)
    rows.append(dict(test=tag,status='PASS_ERROR_PROPAGATION_AND_EXACT_PREFIX',image=str(p),sha256=digest,write_boundary=at,lba=lba,partial_bytes=prefix,fsck_status=ck.returncode,decoder_errors=info['errors'],filesystem_clean=not info['errors'] and ck.returncode==0))
    expected[lba]=data
    save()
    if len(rows)%50==0:print('fault boundaries checked',len(rows),flush=True)
   save()
except BaseException as e:
 rows.append(dict(status='STOP',error=str(e)));save();raise
save();print('PASS injected failures',len(rows),flush=True)
