#!/usr/bin/env python3
"""B1 one-disk derivative: validated FAT16 boot + independently generated FAT32."""
import stat,os
import argparse,struct,json,hashlib,shutil
from pathlib import Path
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('boot_disk',type=Path);ap.add_argument('fat32',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args();assert str(Path(__file__).resolve()).startswith('/tmp/') and str(a.output.resolve()).startswith('/tmp/') and not a.output.exists() and not a.output.is_symlink() and not a.output.with_suffix('.json').exists()
for p in [a.boot_disk,a.fat32]:assert not p.is_symlink() and stat.S_ISREG(p.stat().st_mode)
b=bytearray(a.boot_disk.read_bytes());start=(len(b)//512+2047)//2048*2048;count=a.fat32.stat().st_size//512;assert start+count<0x10000000
b[462:478]=struct.pack('<B3sB3sII',0,b'\xfe\xff\xff',12,b'\xfe\xff\xff',start,count)
with a.output.open('xb') as f:
 f.write(b);f.truncate(start*512);f.seek(start*512)
 with a.fat32.open('rb') as src:shutil.copyfileobj(src,f)
with a.output.open('r+b') as f:
 for sector in (0,6):f.seek((start+sector)*512+28);f.write(struct.pack('<I',start))
with a.output.open('rb') as f:h=hashlib.file_digest(f,'sha256').hexdigest()
a.output.with_suffix('.json').write_text(json.dumps(dict(stage='B1 READ ONLY TEST; not accepted combined system',sha256=h,physical_disks=1,fat16=dict(start=2048,sectors=len(b)//512-2048),fat32=dict(start=start,sectors=count),inputs=[str(a.boot_disk),str(a.fat32)]),indent=2)+'\n');print('Built read-only combined derivative',h)
