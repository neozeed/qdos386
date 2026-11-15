#!/usr/bin/env python3
from guest import *
import struct,hashlib,subprocess
results=[]
for answer in ['N','Q','DEFAULT','EOF','Y','A']:
 boot=B/('products/repair-final-'+answer+'-boot.img');data=B/('products/repair-final-'+answer+'.r7-disposable.img')
 with boot.open('xb') as f:f.write((B/'products/r7a-tools-final.img').read_bytes())
 buf=bytearray((B/'products/r7a-native-secondary.r7-disposable.img').read_bytes());struct.pack_into('<I',buf,(67584+1)*512+488,0)
 with data.open('xb') as f:f.write(buf)
 before=hashlib.sha256(buf).hexdigest();g=None
 try:
  g=Guest(boot,'repair-final-'+answer,extra=['-drive',f'file={data},format=raw,if=ide,index=1'])
  g.type('RUN /CHKDSK.AOU --image ide1 --partition 2 --repair --test-disposable\n');g.wait(lambda s:'Fix (Y/N/A/Q)?' in s,30)
  if answer=='EOF':g.key('ctrl-d')
  else:g.type(('' if answer=='DEFAULT' else answer)+'\n')
  g.wait(g.prompt,20)
  text=(g.log/'debug.txt').read_text();expect=2 if answer in ['Y','A'] else 1
  assert 'exit status '+str(expect) in text,text
 finally:
  if g:g.close()
 after=data.read_bytes()
 if answer not in ['Y','A']:assert after==buf
 else:
  changed=[i for i,(x,y) in enumerate(zip(buf,after)) if x!=y]
  assert changed and all((67584+1)*512<=i<(67584+2)*512 or (67584+7)*512<=i<(67584+8)*512 for i in changed)
  p=B/('products/repair-final-'+answer+'-partition.img');p.write_bytes(after[67584*512:]);q=subprocess.run(['fsck.fat','-n',str(p)],capture_output=True,text=True,timeout=30);(B/'logs'/('repair-final-'+answer+'-fsck.log')).write_text(q.stdout+q.stderr);assert q.returncode==0,q.stdout
 results.append({'answer':answer,'exit':expect,'before':before,'after':hashlib.sha256(after).hexdigest(),'fsck':0 if expect==2 else 'corruption deliberately unchanged'})
 (B/'NATIVE-FINAL-REPAIR-RESULTS.json').write_text(json.dumps(results,indent=2)+'\n');print(results[-1],flush=True)
