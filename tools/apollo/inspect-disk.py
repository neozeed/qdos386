#!/usr/bin/env python3
"""Read-only MBR/BPB inspection; optional exclusive extraction to a new ordinary file."""
import argparse,json,struct,hashlib,os,stat
from pathlib import Path
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('image',type=Path);ap.add_argument('--extract',type=Path);ap.add_argument('--partition',type=int,choices=(1,2,3,4),default=1);a=ap.parse_args();assert a.image.is_file() and Path('/dev') not in a.image.resolve().parents
size=a.image.stat().st_size;assert size%512==0
with a.image.open('rb') as f:
 s=f.read(512);assert s[510:]==b'\x55\xaa';parts=[]
 for i in range(4):
  flag,_,kind,_,start,count=struct.unpack_from('<B3sB3sII',s,446+16*i)
  if not kind:continue
  assert flag in (0,128) and start>0 and count>0 and start+count<=size//512
  assert all(start>=p['start']+p['sectors'] or p['start']>=start+count for p in parts)
  f.seek(start*512);b=f.read(512);assert b[510:]==b'\x55\xaa'
  parts.append(dict(index=i+1,type=kind,active=flag==128,start=start,sectors=count,bps=struct.unpack_from('<H',b,11)[0],spc=b[13],hidden=struct.unpack_from('<I',b,28)[0]))
 if a.extract:
  p=next(p for p in parts if p['index']==a.partition);out=a.extract.resolve();assert Path('/dev') not in out.parents and not out.exists()
  fd=os.open(out,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o644)
  with os.fdopen(fd,'wb') as dst:
   assert stat.S_ISREG(os.fstat(dst.fileno()).st_mode);f.seek(p['start']*512);left=p['sectors']*512
   while left:
    data=f.read(min(left,1048576));assert data;dst.write(data);left-=len(data)
with a.image.open('rb') as f:h=hashlib.file_digest(f,'sha256').hexdigest()
print(json.dumps(dict(image=str(a.image.resolve()),bytes=size,sha256=h,partitions=parts),indent=2))
