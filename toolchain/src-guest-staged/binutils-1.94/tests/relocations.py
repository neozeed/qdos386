#!/usr/bin/env python3
"""Compare synthetic NS32k relocation fixtures with binutils 2.27."""
import argparse, pathlib, struct, subprocess, os
p=argparse.ArgumentParser();p.add_argument('ld');p.add_argument('oracle');p.add_argument('out',type=pathlib.Path);a=p.parse_args()
a.ld=str(pathlib.Path(a.ld).resolve());a.oracle=str(pathlib.Path(a.oracle).resolve());a.out.mkdir(parents=True,exist_ok=True)
def run(args,ok=True):
 r=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 if ok and r.returncode:raise RuntimeError((args,r.stderr.decode()))
 return r
script=a.out/'oracle.ld';script.write_text('SECTIONS { .text 0 : { *(.text) } .data ALIGN(16) : { *(.data) } .bss : { *(.bss) *(COMMON) } }')
def obj(path,storage=0,length=0,pc=False,value=24,external=True,withrel=True):
 text=bytearray(8);size=1<<length
 if storage==1:text[0]={1:0,2:0x80,4:0xc0}[size]
 data=b'DATA1234';strings=b'\0target\0';symbols=struct.pack('<IBBHI',5,3,0,0,value&0xffffffff)
 rel=struct.pack('<II',0,((storage<<5)|(length<<1)|int(pc)|(8 if external else 0))<<24 | (0 if external else 2)) if withrel else b''
 h=struct.pack('<8I',0o407,8,8,0,len(symbols),0,len(rel),0)
 path.write_bytes(h+text+data+rel+symbols+struct.pack('<I',4+len(strings))+strings)
 q=path.with_suffix('.netbsd.o');q.write_bytes(bytes.fromhex('00890107')+path.read_bytes()[4:]);return q
def raw(path):
 d=path.read_bytes();h=struct.unpack('<8I',d[:32]);assert h[6:]==(0,0);return d[32:32+h[1]+h[2]]
count=0
for storage in range(3):
 for length in range(3):
  for pc in (False,True):
   for value in (24,0):
    n=f's{storage}-l{length}-p{int(pc)}-v{value}';o=a.out/(n+'.o');net=obj(o,storage,length,pc,value)
    old=a.out/(n+'.aout');ref=a.out/(n+'.bin')
    run([a.ld,'-N','-o',str(old),str(o)])
    run([a.oracle,'--oformat=binary','-T',str(script),'-o',str(ref),str(net)])
    assert raw(old)==ref.read_bytes(),n
    part=a.out/(n+'.part.o');final=a.out/(n+'.part.aout')
    run([a.ld,'-r','-o',str(part),str(o)]);run([a.ld,'-N','-o',str(final),str(part)])
    assert raw(final)==raw(old),n+' partial'
    count+=1
# Move a local data reference by placing another input object before it.
for storage in range(3):
 for length in range(3):
  for pc in (False,True):
   n=f'local-s{storage}-l{length}-p{int(pc)}'
   o=a.out/(n+'.o');obj(o,storage,length,pc,0,False)
   d=bytearray(o.read_bytes());size=1<<length
   # Local data addends include the original data VMA (8).
   encoded=(8).to_bytes(size,'big' if storage<2 else 'little')
   if storage==1:encoded=bytes([encoded[0]|{1:0,2:0x80,4:0xc0}[size]])+encoded[1:]
   d[32:32+size]=encoded
   # Local r_index N_DATA; remove symbols to prevent duplicate absolute defs.
   h=list(struct.unpack('<8I',d[:32]));h[4]=0
   d=bytearray(struct.pack('<8I',*h))+d[32:56]+struct.pack('<II',0,((storage<<5)|(length<<1)|int(pc))<<24|6)+struct.pack('<I',4)
   o.write_bytes(d);net=o.with_suffix('.netbsd.o');net.write_bytes(bytes.fromhex('00890107')+d[4:])
   prefix=a.out/'prefix.o';prefix.write_bytes(struct.pack('<8I',0o407,8,0,0,0,0,0,0)+b'PREFIX00'+struct.pack('<I',4))
   prefnet=a.out/'prefix.netbsd.o';prefnet.write_bytes(bytes.fromhex('00890107')+prefix.read_bytes()[4:])
   old=a.out/(n+'.aout');ref=a.out/(n+'.bin');part=a.out/(n+'.part.o');final=a.out/(n+'.part.aout')
   run([a.ld,'-N','-o',str(old),str(prefix),str(o)])
   run([a.oracle,'--oformat=binary','-T',str(script),'-o',str(ref),str(prefnet),str(net)])
   assert raw(old)==ref.read_bytes(),n
   run([a.ld,'-r','-o',str(part),str(prefix),str(o)])
   run([a.ld,'-N','-o',str(final),str(part)])
   assert raw(final)==raw(old),n+' partial'
   count+=1
# External PC-relative references at nonzero input offsets, including negative results.
for storage in range(3):
 for length in range(3):
  n=f'external-shift-s{storage}-l{length}'
  o=a.out/(n+'.o');net=obj(o,storage,length,True,0)
  old=a.out/(n+'.aout');ref=a.out/(n+'.bin');part=a.out/(n+'.part.o');final=a.out/(n+'.part.aout')
  run([a.ld,'-N','-o',str(old),str(prefix),str(o)])
  run([a.oracle,'--oformat=binary','-T',str(script),'-o',str(ref),str(prefnet),str(net)])
  assert raw(old)==ref.read_bytes(),n
  run([a.ld,'-r','-o',str(part),str(prefix),str(o)])
  run([a.ld,'-N','-o',str(final),str(part)])
  assert raw(final)==raw(old),n+' partial'
  count+=1
# A displacement must fail rather than silently truncate outside its range.
for storage,value in [(1,64),(0,256),(2,256)]:
 o=a.out/f'overflow-{storage}.o';obj(o,storage,0,False,value)
 dest=o.with_suffix('.aout');r=run([a.ld,'-N','-o',str(dest),str(o)],False)
 assert r.returncode!=0 and not dest.exists(),(storage,r.stderr)
print(f'PASS: {count} oracle comparisons, {count} partial-link round trips, 3 overflow rejections')
