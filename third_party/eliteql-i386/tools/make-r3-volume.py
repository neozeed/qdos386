#!/usr/bin/env python3
"""Copy the supplied public FAT16 volume and add ELITE.AOU to its root.
Never touches a source volume or redistributes the restricted system volume.
"""
from pathlib import Path
import struct,sys,shutil
r=Path(__file__).resolve().parent.parent;osroot=Path(sys.argv[1]).resolve();d=r/'build/os-session';d.mkdir(exist_ok=True)
b=bytearray((osroot/'volumes/qdos386-public.img').read_bytes());bps=struct.unpack_from('<H',b,11)[0];spc=b[13];reserved=struct.unpack_from('<H',b,14)[0];nf=b[16];entries=struct.unpack_from('<H',b,17)[0];spf=struct.unpack_from('<H',b,22)[0]
fat=reserved*bps;root=(reserved+nf*spf)*bps;data=root+entries*32;cs=spc*bps
payload=(r/'build/aout/ELITE.AOU').read_bytes();need=(len(payload)+cs-1)//cs
free=[c for c in range(2,(len(b)-data)//cs+2) if struct.unpack_from('<H',b,fat+c*2)[0]==0][:need];assert len(free)==need
for i,c in enumerate(free):
 v=free[i+1] if i+1<len(free) else 65535
 for j in range(nf):struct.pack_into('<H',b,fat+j*spf*bps+c*2,v)
 chunk=payload[i*cs:(i+1)*cs];off=data+(c-2)*cs;b[off:off+len(chunk)]=chunk
for off in range(root,root+entries*32,32):
 if b[off] in [0,229]:break
else:raise RuntimeError('public root directory full')
assert not any(b[q:q+11]==b'ELITE   AOU' for q in range(root,root+entries*32,32))
e=bytearray(32);e[:11]=b'ELITE   AOU';e[11]=32;struct.pack_into('<H',e,26,free[0]);struct.pack_into('<I',e,28,len(payload));b[off:off+32]=e
(d/'public-elite.img').write_bytes(b)
for name in ['bios.bin','vgabios.bin']:shutil.copyfile(osroot/'kernels'/name,d/name)
shutil.copyfile(osroot/'kernels/qdos386-vga.img',d/'kernel.img')
(d/'run.ini').write_text('[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 16M\nvga_mem_size = 256K\nhda = kernel.img\nhdb = public-elite.img\n[display]\nwidth = 720\nheight = 480\n[cpu]\ngen = 3\nfpu = 1\n')
print(d)
