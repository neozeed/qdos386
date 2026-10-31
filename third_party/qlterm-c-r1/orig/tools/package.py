#!/usr/bin/env python3
"""Create the source release with QDOS02 metadata for the native executable."""
from pathlib import Path
import zipfile,struct,hashlib
root=Path(__file__).resolve().parents[1]
out=root/'dist'/'QLTERM-C-R1-FULL-SOURCE.zip';out.parent.mkdir(exist_ok=True)
files=[root/x for x in ['README.md','PORTING.md','VALIDATION.md','LICENSE','Makefile','qlterm']]
for directory in ['src','reference','tools','tests','validation']:
 files += [p for p in (root/directory).glob('*') if p.is_file() and (p.suffix in ['.c','.h','.py','.md','.txt','.log','.ASM','.asm','.inc','.mac','.lib','.IN','.LINK'] or p.name in ['LICENSE','MultiConfig02'])]
files=sorted(set(files));manifest=[]
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
 for p in files:
  rel=p.relative_to(root);data=p.read_bytes();info=zipfile.ZipInfo('qltc/'+rel.as_posix(),(2026,10,29,23,0,0));info.compress_type=zipfile.ZIP_DEFLATED
  info.external_attr=0o100644<<16
  if p.name=='qlterm' and p.parent==root:
   assert data[-8:-4]==b'XTcc';ds=struct.unpack('>I',data[-4:])[0]
   header=bytearray(64);struct.pack_into('>I',header,0,len(data));header[5]=1
   struct.pack_into('>I',header,6,ds);struct.pack_into('>H',header,14,6);header[16:22]=b'qlterm'
   info.extra=struct.pack('<HH',0xfb4a,72)+b'QDOS02\0\0'+header
  z.writestr(info,data);manifest.append(hashlib.sha256(data).hexdigest()+'  '+rel.as_posix())
 z.writestr('qltc/SHA256SUMS','\n'.join(manifest)+'\n')
with zipfile.ZipFile(out) as z:assert z.testzip() is None
print(out)
print('SHA256',hashlib.sha256(out.read_bytes()).hexdigest())
