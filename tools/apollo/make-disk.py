#!/usr/bin/env python3
"""PRIVATE Apollo A: deterministic new ordinary MBR disk, never overwrite a source."""
import argparse,hashlib,json,os,stat,struct,subprocess,tempfile
from pathlib import Path

def sha(b):return hashlib.sha256(b).hexdigest()
def u16(b,o):return struct.unpack_from('<H',b,o)[0]
def u32(b,o):return struct.unpack_from('<I',b,o)[0]
def add_package(v,data):
 bps=u16(v,11);spc=v[13];reserved=u16(v,14);nf=v[16];spf=u16(v,22);rootent=u16(v,17);total=u16(v,19) or u32(v,32)
 assert bps==512 and nf==2 and spc and spc<=128 and not spc&(spc-1)
 roots=(rootent*32+511)//512;ds=reserved+nf*spf+roots;clusters=(total-ds)//spc;assert 4085<=clusters<=65524 and total*512==len(v)
 fat=bytearray(v[reserved*512:(reserved+spf)*512]);assert fat==v[(reserved+spf)*512:(reserved+2*spf)*512]
 free=[c for c in range(2,clusters+2) if u16(fat,c*2)==0];n=(len(data)+spc*512-1)//(spc*512)
 # Deliberately fragmented loader file, to exercise the actual FAT chain.
 chosen=free[::2][:n];assert len(chosen)==n and n>0
 root=(reserved+nf*spf)*512;slot=None
 for at in range(root,root+rootent*32,32):
  assert v[at:at+11]!=b'QDOS    SYS','Refuse replacing an existing package'
  if slot is None and v[at] in (0,229):slot=at
 assert slot is not None
 for i,c in enumerate(chosen):
  struct.pack_into('<H',fat,c*2,chosen[i+1] if i+1<n else 0xffff)
  block=data[i*spc*512:(i+1)*spc*512];at=(ds+(c-2)*spc)*512;v[at:at+spc*512]=block.ljust(spc*512,b'\0')
 for f in range(nf):v[(reserved+f*spf)*512:(reserved+(f+1)*spf)*512]=fat
 ent=bytearray(32);ent[:11]=b'QDOS    SYS';ent[11]=32;date=((2026-1980)<<9)|(11<<5)|14
 for off in (16,18,24):struct.pack_into('<H',ent,off,date)
 struct.pack_into('<H',ent,26,chosen[0]);struct.pack_into('<I',ent,28,len(data));v[slot:slot+32]=ent
 return dict(spc=spc,total_sectors=total,clusters=clusters,kernel_clusters=chosen,kernel_package_bytes=len(data),kernel_package_sha256=sha(data))

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--boot',required=True,type=Path);ap.add_argument('--package',required=True,type=Path);ap.add_argument('--volume',required=True,type=Path);ap.add_argument('--output',required=True,type=Path);a=ap.parse_args()
 out=a.output.resolve();assert out!=Path('/dev') and Path('/dev') not in out.parents and not out.exists(),'Only a NEW ordinary output file; never /dev/* or overwrite'
 assert not out.with_suffix(out.suffix+'.json').exists(),'Refuse existing manifest'
 for p in [a.volume,a.package,*[a.boot/f for f in ('mbr.bin','vbr.bin','stage2.bin')]]:
  assert p.is_file() and stat.S_ISREG(p.stat().st_mode) and Path('/dev') not in p.resolve().parents,'Ordinary source files only'
 volume=bytearray(a.volume.read_bytes());package=a.package.read_bytes();assert package[0x1ca:0x1d0]==b'QBOOT3' and package[-1:] and len(package)%512==0
 ks,xs=struct.unpack_from('<HH',package,0x1c2);assert ks>0 and 1<=xs<=512 and ks+xs<=896 and len(package)==512*(1+ks+xs)
 s=0
 for (w,) in struct.iter_unpack('<I',package[512:]):s=(s+w)&0xffffffff;s=((s<<1)|(s>>31))&0xffffffff
 assert s==u32(package,0x1c6),'Invalid source kernel package'
 params=add_package(volume,package);mbr=bytearray((a.boot/'mbr.bin').read_bytes());vbr=(a.boot/'vbr.bin').read_bytes();stage=(a.boot/'stage2.bin').read_bytes()
 assert len(mbr)==len(vbr)==512 and len(stage)==16384 and sum(struct.unpack('<4096I',stage))&0xffffffff==0
 start=2048;count=len(volume)//512;diskid=int(sha(package+stage)[:8],16) or 1
 struct.pack_into('<I',mbr,440,diskid);mbr[446:510]=bytes(64);mbr[446:462]=struct.pack('<B3sB3sII',128,b'\xfe\xff\xff',14,b'\xfe\xff\xff',start,count)
 volume[:3]=vbr[:3];volume[62:510]=vbr[62:510];volume[510:512]=b'\x55\xaa';struct.pack_into('<I',volume,28,start)
 disk=bytearray((start+count)*512);disk[:512]=mbr;disk[512:512+len(stage)]=stage;disk[start*512:]=volume
 out.parent.mkdir(parents=True,exist_ok=True);fd=os.open(out,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o644)
 with os.fdopen(fd,'wb') as f:assert stat.S_ISREG(os.fstat(f.fileno()).st_mode);f.write(disk)
 with tempfile.TemporaryDirectory(prefix='apollo-fsck-',dir=out.parent) as tmp:
  p=Path(tmp)/'partition.img';p.write_bytes(volume);check=subprocess.run(['fsck.fat','-n',str(p)],capture_output=True,text=True,timeout=30);assert check.returncode==0,check.stdout+check.stderr
 manifest=dict(private_internal=True,stage='A',image=str(out),image_sha256=sha(disk),bytes=len(disk),physical_disks=1,partition=dict(index=1,type=14,active=True,start=start,sectors=count),disk_id=diskid,filesystem=params,inputs={str(a.volume.resolve()):sha(a.volume.read_bytes()),str(a.package.resolve()):sha(package),**{str((a.boot/f).resolve()):sha((a.boot/f).read_bytes()) for f in ('mbr.bin','vbr.bin','stage2.bin')}},fsck=check.stdout.strip(),boot_contract='R6BI v1/48bytes atphysical0x900; EDD-only; fragmented QDOS.SYS; WIN1=validated boot FAT16')
 manifest_path=out.with_suffix(out.suffix+'.json');assert not manifest_path.exists();manifest_path.write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps({k:manifest[k] for k in ['image','image_sha256','bytes','partition']}))
if __name__=='__main__':main()
