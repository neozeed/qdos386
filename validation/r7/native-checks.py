#!/usr/bin/env python3
from guest import *
import shutil,hashlib
boot=B/'products/r7a-final-native-boot.img';data=B/'products/r7a-final-native-secondary.r7-disposable.img'
assert not boot.exists() and not data.exists()
with boot.open('xb') as f:f.write((B/'products/r7a-tools-final.img').read_bytes())
with data.open('xb') as f:f.truncate(198656*512)
g=None;results=[]
def command(s,expected=0,seconds=90):
 before=len((g.log/'debug.txt').read_bytes());g.command(s,seconds)
 text=(g.log/'debug.txt').read_text(errors='replace')[before:]
 assert ('exit status '+str(expected)) in text,(s,text)
 results.append({'command':s,'exit':expected});(B/'NATIVE-FINAL-RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
try:
 g=Guest(boot,'stage-a-final-native',extra=['-drive',f'file={data},format=raw,if=ide,index=1'])
 command('RUN /FDISK.AOU --image ide0')
 command('RUN /FDISK.AOU --image ide1')
 command('RUN /FORMAT.AOU --image ide0 --partition 1 --fat 16 --test-disposable',1)
 g.type('RUN /FDISK.AOU --image ide1 --edit --test-disposable\n');g.wait(lambda s:'FDISK>' in s,20)
 for s in ['create 1 2048 65536 16','create 2 67584 131072 32','active 1']:
  g.type(s+'\n');time.sleep(.15)
 g.type('write\n');g.wait(g.prompt,30)
 assert 'Partition table written' in (g.log/'debug.txt').read_text()
 command('RUN /FORMAT.AOU --image ide1 --partition 1 --fat 16 --test-disposable')
 command('RUN /FORMAT.AOU --image ide1 --partition 2 --fat 32 --test-disposable')
 command('RUN /CHKDSK.AOU --image ide1 --partition 1')
 command('RUN /CHKDSK.AOU --image ide1 --partition 2')
 command('RUN R20T')
 command('RUN GCC -o /TMP/R7AHI.AOU /HOME/HI.C')
 command('RUN /TMP/R7AHI.AOU')
 g.command('SYNC')
finally:
 if g:g.close()
print('Native utility and FAT16 regression commands passed')
