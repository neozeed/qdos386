#!/usr/bin/env python3
"""Additional formatting/partition preservation checks; run only in /tmp."""
from pathlib import Path
import os,sys,subprocess,struct,json,hashlib
root=Path(__file__).resolve().parents[2];base=Path(sys.argv[1]).resolve();assert str(root).startswith('/tmp/') and str(base).startswith('/tmp/')
source=base/'host-checks-final/clean.r7-disposable.img';out=base/'host-preservation';out.mkdir();rows=[]
def run(n,t,data,args,stdin='',expected=0):
 p=out/(n+'.r7-disposable.img');p.write_bytes(data);q=subprocess.run([str(root/'source/apps/disktools'/t),'--image',str(p),*args],input=stdin,text=True,capture_output=True,timeout=30);(out/(n+'.log')).write_text(q.stdout+q.stderr);assert q.returncode==expected,(n,q.stdout);after=p.read_bytes();rows.append(dict(name=n,exit=q.returncode,before=hashlib.sha256(data).hexdigest(),after=hashlib.sha256(after).hexdigest()));return after
original=source.read_bytes()
x=bytearray(original);x[:8]=b'BOOTCODE';struct.pack_into('<I',x,440,0x33445566)
a=run('mbr-preserve','fdisk-host',x,['--edit','--test-disposable'],'active 2\nwrite\n');assert a[:446]==x[:446]
for part,start,code,bits in [(1,2048,62,16),(2,67584,90,32)]:
 x=bytearray(original);x[start*512+code:start*512+code+8]=b'VBR-CODE'
 a=run('vbr-preserve'+str(bits),'format-host',x,['--partition',str(part),'--fat',str(bits),'--test-disposable']);assert a[:512]==x[:512];assert a[start*512+code:start*512+510]==x[start*512+code:start*512+510]
x=bytearray(original);x[(67584+2)*512]=99;a=run('reserved-loader-refuse','format-host',x,['--partition','2','--fat','32','--test-disposable'],expected=3);assert a==x
x=bytearray(original);struct.pack_into('<I',x,(67584+1)*512+488,0)
p=out/'repair-partial.r7-disposable.img';p.write_bytes(x);q=subprocess.run([str(root/'source/apps/disktools/chkdsk-host'),'--image',str(p),'--partition','2','--repair','--test-disposable'],input='Y\n',text=True,capture_output=True,timeout=30,env=dict(os.environ,QDOS_DISK_TEST_FAIL_AFTER='1'));assert q.returncode==3 and 'possible partial update' in q.stdout;(out/'repair-partial.log').write_text(q.stdout);rows.append(dict(name='repair-partial',exit=q.returncode))
# Linux mkfs independently produces a good FAT32 BPB/metadata source.
p=out/'linux.partition';p.write_bytes(bytes(131072*512));q=subprocess.run(['mkfs.fat','-F','32',str(p)],capture_output=True,text=True,timeout=30);assert q.returncode==0
x=bytearray(original);x[67584*512:]=p.read_bytes();a=run('linux-fat32','chkdsk-host',x,['--partition','2']);assert a==x
# Absent boot/FSInfo locations are not supported by this checker, diagnosed.
(out/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print('PASS',len(rows),'additional checks')
