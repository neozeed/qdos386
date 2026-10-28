#!/usr/bin/env python3
"""Extract this toolchain's zero-based, fully linked OMAGIC image.

Text includes any padding up to data. BSS is excluded: the emulator starts
with zeroed RAM. This is deliberately not a general Unix a.out loader.
"""
import argparse, pathlib, struct
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('input',type=pathlib.Path);p.add_argument('output',type=pathlib.Path)
p.add_argument('--zero-bss',action='store_true',help='append BSS zeros (normally unnecessary)')
a=p.parse_args();d=a.input.read_bytes()
if len(d)<32:p.error('truncated a.out header')
h=list(struct.unpack('<8I',d[:32]));magic=h[0]&65535
if magic!=0o407:p.error('expected little-endian OMAGIC from ns32k-ld -N')
_,text,data,bss,syms,entry,tr,dr=h
if tr or dr:p.error('input still contains relocations; perform the final link first')
if not text or entry>=text:p.error('entry point is outside the zero-based text segment')
if text+data+bss>8*1024*1024:p.error('image plus BSS exceeds emulator RAM')
if len(d)<32+text+data+syms:p.error('truncated sections or symbol table')
raw=d[32:32+text+data]
if a.zero_bss:raw+=bytes(bss)
a.output.write_bytes(raw)
print(f'{a.output}: {len(raw)} bytes, text={text}, data={data}, bss={bss}, entry=0x{entry:x}')
