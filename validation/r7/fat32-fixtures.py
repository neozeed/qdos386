#!/usr/bin/env python3
"""Independent FAT32 BPBs (Linux mkfs + separate assembler), known fragmented data."""
import stat,os
import argparse,json,struct,subprocess,hashlib
from pathlib import Path
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path);ap.add_argument('--fats',type=int,choices=(1,2),default=2);a=ap.parse_args();assert str(Path(__file__).resolve()).startswith("/tmp/") and str(a.output.resolve()).startswith("/tmp/");a.output.mkdir(parents=True,exist_ok=False)
def u16(b,o):return struct.unpack_from('<H',b,o)[0]
def u32(b,o):return struct.unpack_from('<I',b,o)[0]
rows=[]
for name,mb,spc,linux in [('linux-spc1',64,1,True),('linux-spc2',128,2,True),('independent-root7',64,1,False)]:
 p=a.output/(name+'.img')
 with p.open('xb') as f:f.truncate(mb*1024*1024)
 if linux:subprocess.run(['mkfs.fat','--invariant','-F','32','-S','512','-s',str(spc),'-f',str(a.fats),'-R','32','-i','A6010032','-n','R6FAT32',str(p)],check=True,capture_output=True)
 with p.open('r+b') as f:
  if linux:
   b=bytearray(f.read(512));reserved=u16(b,14);nf=b[16];spf=u32(b,36);total=u32(b,32);root=u32(b,44)
  else:
   total=mb*2048;reserved=32;nf=a.fats;spf=1009 if nf==2 else 1024;root=7;b=bytearray(512);b[:3]=b'\xeb\x58\x90';b[3:11]=b'APOLLO6 ';struct.pack_into('<HBHBHHBHHHII',b,11,512,spc,reserved,nf,0,0,248,0,63,255,0,total);struct.pack_into('<IHHIHH',b,36,spf,0,0,root,1,6);b[64]=128;b[66]=41;struct.pack_into('<I',b,67,0xA6010032);b[71:82]=b'R6FAT32    ';b[82:90]=b'FAT32   ';b[510:]=b'\x55\xaa'
  data=reserved+nf*spf;clusters=(total-data)//spc;assert 65525<=clusters<0xffffff0
  fat=bytearray(spf*512);struct.pack_into('<II',fat,0,0x0ffffff8,0xffffffff)
  payload=bytes((i*37+19)&255 for i in range(1703));chain=[70000,120,70001,150][: (len(payload)+spc*512-1)//(spc*512)];used={root,17,19,*chain}
  for c in [root,17,19]:struct.pack_into('<I',fat,c*4,0x0fffffff)
  for i,c in enumerate(chain):struct.pack_into('<I',fat,c*4,(chain[i+1] if i+1<len(chain) else 0x0fffffff)|0xa0000000)
  # High reserved bits are deliberately nonzero in file FAT entries.
  def ent(name,attr,c,size=0):
   e=bytearray(32);base,_,ext=name.partition('.');e[:11]=(base.ljust(8)+ext.ljust(3)).encode();e[11]=attr;struct.pack_into('<H',e,20,c>>16);struct.pack_into('<H',e,26,c&65535);struct.pack_into('<I',e,28,size);return e
  def put(c,d):f.seek((data+(c-2)*spc)*512);f.write(d.ljust(spc*512,b'\0'))
  rootdata=ent('R6FAT32',8,0)+ent('FRAG.BIN',32,chain[0],len(payload))+ent('NEST',16,17)+ent('EMPTY',16,19)
  put(root,rootdata)
  dot=lambda c,parent: ent('',16,c) # explicitly encoded below
  def dirdata(c,parent):
   x=ent('DOT',16,c);x[:11]=b'.          ';y=ent('DOTDOT',16,parent);y[:11]=b'..         ';return x+y
  put(17,dirdata(17,0)+ent('TINY.TXT',32,0,0));put(19,dirdata(19,0))
  for i,c in enumerate(chain):put(c,payload[i*spc*512:(i+1)*spc*512])
  info=bytearray(512);struct.pack_into('<I',info,0,0x41615252);struct.pack_into('<III',info,484,0x61417272,clusters-len(used),2);struct.pack_into('<I',info,508,0xaa550000)
  f.seek(0);f.write(b);f.seek(6*512);f.write(b);f.seek(512);f.write(info);f.seek(7*512);f.write(info)
  for copy in range(nf):f.seek((reserved+copy*spf)*512);f.write(fat)
 check=subprocess.run(['fsck.fat','-n',str(p)],capture_output=True,text=True);assert check.returncode==0,check.stdout+check.stderr
 rows.append(dict(name=name,path=str(p.resolve()),origin='Linux mkfs.fat BPB' if linux else 'Independent Python BPB assembly',spc=spc,fat_copies=nf,root=root,clusters=clusters,fragmented_chain=chain,content_sha256=hashlib.sha256(payload).hexdigest(),content_bytes=len(payload),fsck=check.stdout.strip()))
(a.output/'manifest.json').write_text(json.dumps(dict(status='PASS',fixtures=rows),indent=2)+'\n');print('PASS independent FAT32 fixtures',len(rows))
