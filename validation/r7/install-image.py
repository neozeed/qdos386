#!/usr/bin/env python3
"""Create a disposable derivative of the accepted FAT16 disk. Never overwrite input.
Replace QDOS.SYS only in the new image; install named root-directory utilities.
"""
import argparse,hashlib,struct,importlib.util,json,os,stat
from pathlib import Path
ap=argparse.ArgumentParser();ap.add_argument('--input',required=True,type=Path);ap.add_argument('--output',required=True,type=Path);ap.add_argument('--package',required=True,type=Path);ap.add_argument('--apps',required=True,type=Path);ap.add_argument('--probe',type=Path);ap.add_argument('--probe-name',default='DTABI.AOU',choices=['DTABI.AOU','F32IO.AOU']);a=ap.parse_args()
root=Path(__file__).resolve().parents[2]
assert not a.output.exists() and not a.output.is_symlink()
inputs=[a.input,a.package]+[a.apps/(n+'.aout') for n in ['FDISK','FORMAT','CHKDSK']]
if a.probe:inputs.append(a.probe)
for p in inputs:assert not p.is_symlink() and stat.S_ISREG(p.stat().st_mode)
disk=bytearray(a.input.read_bytes());start,count=struct.unpack_from('<II',disk,454);assert start==2048 and count==65536 and len(disk)==(start+count)*512
v=bytearray(disk[start*512:]);u16=lambda o:struct.unpack_from('<H',v,o)[0];spc=v[13];res=u16(14);spf=u16(22);entries=u16(17);assert v[16]==2 and u16(11)==512
rootpos=(res+2*spf)*512;ds=res+2*spf+(entries*32+511)//512;clusters=(count-ds)//spc;fat=bytearray(v[res*512:(res+spf)*512]);assert fat==v[(res+spf)*512:(res+2*spf)*512]
def get(c):return struct.unpack_from('<H',fat,c*2)[0]
def put(c,n):struct.pack_into('<H',fat,c*2,n)
def install(name,body,replace=False):
 raw=(name.split('.')[0].ljust(8)+name.split('.')[1].ljust(3)).encode();slot=None
 for at in range(rootpos,rootpos+entries*32,32):
  if v[at:at+11]==raw:
   assert replace;slot=at;c=struct.unpack_from('<H',v,at+26)[0];seen=set()
   while c<0xfff8:
    assert 2<=c<clusters+2 and c not in seen;seen.add(c);n=get(c);put(c,0);c=n
   break
 if slot is None:
  slot=next(at for at in range(rootpos,rootpos+entries*32,32) if v[at] in (0,229))
 n=(len(body)+spc*512-1)//(spc*512);free=[c for c in range(2,clusters+2) if not get(c)][:n];assert len(free)==n
 for i,c in enumerate(free):
  put(c,free[i+1] if i+1<n else 0xffff);at=(ds+(c-2)*spc)*512;v[at:at+spc*512]=body[i*spc*512:(i+1)*spc*512].ljust(spc*512,b'\0')
 e=bytearray(32);e[:11]=raw;e[11]=32;date=((2026-1980)<<9)|(11<<5)|15
 for off in (16,18,24):struct.pack_into('<H',e,off,date)
 struct.pack_into('<H',e,26,free[0] if free else 0);struct.pack_into('<I',e,28,len(body));v[slot:slot+32]=e
package=a.package.read_bytes();assert package[0x1ca:0x1d0]==b'QBOOT3';ks,xs=struct.unpack_from('<HH',package,0x1c2);assert len(package)==512*(1+ks+xs)
install('QDOS.SYS',package,True)
for n in ['FDISK','FORMAT','CHKDSK']:install(n+'.AOU',(a.apps/(n+'.aout')).read_bytes())
if a.probe:install(a.probe_name,a.probe.read_bytes())
for i in range(2):v[(res+i*spf)*512:(res+(i+1)*spf)*512]=fat
disk[start*512:]=v
with a.output.open('xb') as f:f.write(disk)
manifest={'inputs':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},'output':str(a.output),'sha256':hashlib.sha256(disk).hexdigest(),'boot_gap_and_vbr_preserved':True}
a.output.with_suffix('.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(manifest))
