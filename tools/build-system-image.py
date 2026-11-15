"""Current image assembly; historical payload is an explicit optional input."""
import hashlib,json,shutil,struct,sys,tempfile
from pathlib import Path

def build(b):
 R=Path(__file__).resolve().parents[1];o=b.out;products=o/'products';work=Path(tempfile.mkdtemp(prefix='image-',dir=o/'tmp'));seed=b.a.payload
 if seed:
  seed=seed.resolve()
  if not seed.is_file() or seed.is_symlink():raise ValueError('Private payload must be an ordinary FAT16 disk image')
  raw=seed.read_bytes();assert len(raw)==67584*512 and struct.unpack_from('<II',raw,454)==(2048,65536)
  # Prove the supplied recovery loader matches the freshly rebuilt BIOS stages.
  boot=products/'apollo-boot'
  assert raw[:440]==(boot/'mbr.bin').read_bytes()[:440]
  assert raw[512:512+16384]==(boot/'stage2.bin').read_bytes()
  vbr=(boot/'vbr.bin').read_bytes();at=2048*512
  assert raw[at:at+3]==vbr[:3] and raw[at+62:at+512]==vbr[62:]
 else:
  # Source-owned minimal bootable system. No recovered applications are invented.
  payload=work/'payload';payload.mkdir();(payload/'STARTUP.CMD').write_text('SET PATH=/BIN;/SDK/BIN\nSET HOME=/HOME\nSET TEMP=/TMP\nSET TMP=/TMP\nSET INCLUDE=/SDK/INCLUDE\nSET LIB=/SDK/LIB\n')
  (payload/'HI.C').write_text('#include <stdio.h>\nint main(void) { puts("QDOS/386 native GCC works!"); return 0; }\n')
  entries=['D\t/BIN','D\t/SDK','D\t/SDK/INCLUDE','D\t/SDK/LIB','D\t/ETC','D\t/HOME','D\t/TMP']
  for target,p in [('/SHELL.AOU',products/'shell.aout'),('/BIN/SH.AOU',products/'sh.aout'),('/ETC/STARTUP.CMD',payload/'STARTUP.CMD'),('/HOME/HI.C',payload/'HI.C')]:entries.append('F\t'+target+'\t'+str(p))
  for p in sorted((R/'source/sdk/include').iterdir()):
   if p.is_file():entries.append('F\t/SDK/INCLUDE/'+p.name.upper()+'\t'+str(p))
  for p in sorted((o/'sdk/lib').iterdir()):
   if p.suffix in ('.a','.o'):entries.append('F\t/SDK/LIB/'+p.name.upper()+'\t'+str(p))
  manifest=work/'minimal.tsv';manifest.write_text('\n'.join(entries)+'\n');volume=work/'minimal.disposable.img'
  with volume.open('xb') as f:f.truncate(65536*512)
  b.run(['mkfs.fat','--invariant','-F','16','-s','4','-n','QDOS16',volume])
  b.run([products/'populate','--disposable',volume,'--manifest',manifest]);seed=work/'fallback.disposable.img'
  b.run([sys.executable,R/'tools/apollo/make-disk.py','--boot',products/'apollo-boot','--package',products/'qdos386-vga.img','--volume',volume,'--output',seed])
 result=work/'combined'
 b.run([sys.executable,R/'validation/r7/fat32-combined.py','--output',result,'--fallback',seed,'--package',products/'qdos386-vga.img','--apps',products,'--channels',products/'F32IO.AOU','--live',products/'F32LIVE.AOU','--populator',products/'populate','--sdk-libs',o/'sdk/lib'])
 disk=products/'qdos386-system.img';b.run(['cp','--sparse=always',result/'qdos386-r7b2-fat32.disposable.img',disk])
 receipt=json.loads((result/'receipt.json').read_text());receipt.update(disk=str(disk),payload_mode='private-reference' if b.a.payload else 'source-only-minimal',payload_source=str(b.a.payload.resolve()) if b.a.payload else None,work=str(work))
 (products/'image.json').write_text(json.dumps(receipt,indent=2)+'\n')
 return [disk,products/'image.json']
