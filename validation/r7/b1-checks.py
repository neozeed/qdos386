#!/usr/bin/env python3
from guest import *
import struct,hashlib,subprocess
R=Path(__file__).resolve().parents[2];assert str(R).startswith('/tmp/')
records=json.loads((B/"B1-RESULTS.json").read_text()) if len(sys.argv)>1 and (B/"B1-RESULTS.json").exists() else []
def u16(b,o):return struct.unpack_from('<H',b,o)[0]
def u32(b,o):return struct.unpack_from('<I',b,o)[0]
def put(b,o,n):struct.pack_into('<I',b,o,n)
for case in (sys.argv[1:] or ['linux-spc1','linux-spc2','root7','active-fat1','bad-chain','bad-info']):
 f=B/'fat32-fixtures'/({'linux-spc1':'linux-spc1','linux-spc2':'linux-spc2'}.get(case,'independent-root7')+'.img');v=bytearray(f.read_bytes());mode='';res=u16(v,14);spf=u32(v,36)
 if case=='active-fat1':
  for boot in [0,6]:struct.pack_into('<H',v,boot*512+40,129)
  v[res*512:(res+spf)*512]=bytes(spf*512)
 if case=='bad-chain':
  for fat in range(2):put(v,(res+fat*spf)*512+70000*4,70000)
  mode=' bad'
 if case=='bad-info':put(v,512,0);mode=' fail'
 vol=B/('products/b1-'+case+'-volume.img')
 with vol.open('xb') as dest:dest.write(v)
 hostmode='bad-file' if case=='bad-chain' else 'bad-mount' if case=='bad-info' else 'ok'
 payload=B/('products/b1-'+case+'.payload');q=subprocess.run([str(B/'products/fat32-read-host'),str(vol),hostmode,str(payload)],capture_output=True,text=True,timeout=30);(B/'logs'/('b1-'+case+'-host.log')).write_text(q.stdout+q.stderr);assert q.returncode==0,(case,q.stdout,q.stderr)
 if not mode:assert hashlib.sha256(payload.read_bytes()).hexdigest()==hashlib.sha256(bytes((i*37+19)&255 for i in range(1703))).hexdigest()
 image=B/('products/b1-'+case+'.img');subprocess.run([sys.executable,str(R/'validation/r7/make-ro-combined.py'),str(B/'products/b1-boot-probe.img'),str(vol),str(image)],check=True)
 before=image.read_bytes();g=None
 try:
  g=Guest(image,'b1-'+case)
  pos=len((g.log/'debug.txt').read_bytes());g.command('RUN /DTABI.AOU'+mode);text=(g.log/'debug.txt').read_text()[pos:];assert 'exit status 0' in text,(case,text)
  if case=='linux-spc1':
   for cmd in ['RUN /DTABI.AOU exit','RUN /DTABI.AOU','RUN R20T','RUN GCC -o /TMP/B1HI.AOU /HOME/HI.C','RUN /TMP/B1HI.AOU']:
    pos=len((g.log/'debug.txt').read_bytes());g.command(cmd);assert 'exit status 0' in (g.log/'debug.txt').read_text()[pos:]
   g.command('SYNC')
 finally:
  if g:g.close()
 after=image.read_bytes();assert after[67584*512:]==before[67584*512:];assert after[:2048*512+512]==before[:2048*512+512]
 if case!='linux-spc1':assert after==before
 fs=[]
 for n,start,count in [(1,2048,65536),(2,67584,len(v)//512)]:
  p=B/('products/b1-'+case+'-part'+str(n)+'.img');p.write_bytes(after[start*512:(start+count)*512]);q=subprocess.run(['fsck.fat','-n',str(p)],capture_output=True,text=True,timeout=30);(B/'logs'/('b1-'+case+'-part'+str(n)+'-fsck.log')).write_text(q.stdout+q.stderr);fs.append(q.returncode)
  if case=='active-fat1' and n==2:
   assert q.returncode==1 and 'FATs differ - using second FAT.' in q.stdout and '5 files, 7/' in q.stdout
   check=subprocess.run([str(B/'stage-a-worktree/source/apps/disktools/chkdsk-host'),'--image',str(image),'--partition','2'],capture_output=True,text=True,timeout=30)
   assert check.returncode==0 and 'errors=0' in check.stdout
  elif not mode or n==1:assert q.returncode==0,(case,n,q.stdout)
 records.append({'case':case,'status':'PASS','kernel_mode':'read-only','host':hostmode,'fat32_unchanged':True,'fsck':fs,'image_sha256':hashlib.sha256(before).hexdigest(),'fat32_sha256':hashlib.sha256(before[67584*512:]).hexdigest()})
 (B/'B1-RESULTS.json').write_text(json.dumps(records,indent=2)+'\n');print(records[-1],flush=True)
