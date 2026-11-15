#!/usr/bin/env python3
"""Check whether differences from the 2.27 image are confined to COMMON relocations."""
import pathlib,struct,argparse
p=argparse.ArgumentParser();p.add_argument('objects',type=pathlib.Path);p.add_argument('reference',type=pathlib.Path);p.add_argument('image',type=pathlib.Path);a=p.parse_args()
names='crt0 file funcs infocom init input interp io jump object options page print property support variable term printf mem string c_getc c_putc planetfa'.split()
objects=[];commons=set()
for n in names:
 d=(a.objects/(n+'.o')).read_bytes();h=list(struct.unpack('<8I',d[:32]));h[0]=int.from_bytes(d[:4],'big')
 text=h[1]-(32 if (h[0]&65535)==0o413 else 0)
 symoff=32+text+h[2]+h[6]+h[7];strings=d[symoff+h[4]:];syms=[]
 for off in range(symoff,symoff+h[4],12):
  index,kind,other,desc,value=struct.unpack('<IBBHI',d[off:off+12]);name=strings[index:].split(b'\0')[0].decode() if index else ''
  syms.append(name)
  if kind==1 and value:commons.add(name)
 objects.append((n,d,h,text,syms))
coverage=set();offset=0;sites=0
for n,d,h,text,syms in objects:
 offset=(offset+7)&~7
 roff=32+text+h[2]
 for i in range(0,h[6],8):
  addr,info=struct.unpack('<II',d[roff+i:roff+i+8]);t=info>>24;index=info&0xffffff
  if t&8 and syms[index] in commons:
   coverage.update(range(offset+addr,offset+addr+(1<<((t>>1)&3))));sites+=1
 offset+=text
x=a.reference.read_bytes();y=a.image.read_bytes();assert len(x)==len(y)
diff={i for i,(j,k) in enumerate(zip(x,y)) if j!=k}
assert diff <= coverage, sorted(diff-coverage)[:20]
assert x[0x47d0:]==y[0x47d0:]
print(f'PASS: {len(diff)} differing bytes are all inside {sites} COMMON-reference relocation fields; complete data segment identical; size {len(x)} bytes')
