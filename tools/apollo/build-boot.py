#!/usr/bin/env python3
"""Build new BIOS stages with existing system binutils; legacy GAS/kernel untouched."""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False);root=Path(__file__).resolve().parents[2];files={}
for name,origin in [('mbr',0x600),('vbr',0x7c00),('stage2',0x1000)]:
 src=root/'source/platform/pc/apollo'/(name+'.S');obj=a.output/(name+'.o');dest=a.output/(name+'.bin')
 subprocess.run(['/usr/bin/as','--32','-o',str(obj),str(src)],check=True)
 subprocess.run(['/usr/bin/ld','-m','elf_i386','--oformat','binary','-Ttext',hex(origin),'-e','start','-o',str(dest),str(obj)],check=True)
 b=dest.read_bytes()
 if name=='stage2':
  assert 24<=len(b)<=16384;b=bytearray(b.ljust(16384,b'\0'));assert b[8:12]==b'R6L1';struct.pack_into('<I',b,16,(-sum(struct.unpack('<4096I',b)))&0xffffffff);dest.write_bytes(b);assert sum(struct.unpack('<4096I',b))&0xffffffff==0
 else:assert len(b)==512 and b[-2:]==b'\x55\xaa'
 files[name]={'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
(a.output/'manifest.json').write_text(json.dumps({'files':files,'assembler':subprocess.check_output(['/usr/bin/as','--version'],text=True).splitlines()[0],'linker':subprocess.check_output(['/usr/bin/ld','--version'],text=True).splitlines()[0]},indent=2)+'\n');print(json.dumps(files))
