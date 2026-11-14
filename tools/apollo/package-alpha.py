#!/usr/bin/env python3
"""Private A-only GateC package; fresh reproduction, never publish or overwrite."""
import argparse,gzip,hashlib,io,json,shutil,subprocess,tarfile
from pathlib import Path
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('destination',type=Path);a=ap.parse_args();r=Path(__file__).resolve().parents[2];ref=r.parent/'reference/qdos386/r6-single-disk-internal';stage=ref/'stage-a';dest=a.destination.resolve()
def sha(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def write(p,d):p.write_text(json.dumps(d,indent=2)+'\n')
def git(*args):return subprocess.check_output(['git',*args],cwd=r)
assert not git('status','--porcelain=v1','-uall'),'Commit C source before packaging'
assert not git('diff','r6a-fat16-single-disk-accepted','HEAD','--','source','Makefile','tools/apollo/build-boot.py','tools/apollo/make-disk.py','scripts/investigate-memory.py','apps','sdk','DEVELOPMENT-POLICY.md'),'A build sources must match frozen tag'
source=git('rev-parse','HEAD').decode().strip();dest.mkdir(parents=True,exist_ok=False)
archive=dest/'qdos386-apollo-private-source.tar.gz';raw=git('archive','--format=tar','--prefix=qdos386-apollo-private/',source);archive.write_bytes(gzip.compress(raw,mtime=0));files={}
with tarfile.open(fileobj=io.BytesIO(raw)) as tf:
 for m in tf:
  if m.isfile():files[m.name.removeprefix('qdos386-apollo-private/')]=hashlib.sha256(tf.extractfile(m).read()).hexdigest()
write(dest/'SOURCE-FILES.json',files)
image=dest/'qdos386-alpha-candidate.img';shutil.copyfile(stage/'qdos386-r6a.img',image);expected='3b3ceb41210c9726af2cd65c55225a5571f9df7193c96d722ce017506560396a';assert sha(image)==expected
for p in (r/'launch/apollo').iterdir():
 if p.is_file():shutil.copy2(p,dest/p.name)
for name in ['README-ALPHA.md','BOOT-AND-DISK.md','FILESYSTEM-LIMITATIONS.md','TEST-RESULTS.md','KNOWN-ISSUES.md','HANDOVER.md','STAGE-A-ACCEPTANCE.md','STAGE-B-INCOMPLETE.md']:shutil.copyfile(r/'reports/r6'/name,dest/name)
shutil.copy2(r/'tools/apollo/inspect-disk.py',dest/'inspect-disk.py')
shutil.copyfile(stage/'IMAGE-MANIFEST.json',dest/'STAGE-A-IMAGE-MANIFEST.json')
(dest/'source-from-r5-canonical.patch').write_bytes(git('diff','--binary','--full-index','cc04021',source))
work=r/'build/r6/alpha-fresh';work.mkdir(exist_ok=False)
with tarfile.open(archive) as tf:tf.extractall(work,filter='data')
fresh=work/'qdos386-apollo-private'
for p,h in files.items():assert sha(fresh/p)==h,p
commands=[['make','apollo-r6','HOSTTOOLS=/data/projects/qdos386/hosttools'],['python3','tools/apollo/build-boot.py','build/alpha-boot'],['python3','tools/apollo/make-disk.py','--boot','build/alpha-boot','--package','source/build/apollo-r6/pc-vga/qdos386-vga.img','--volume',str(r.parent/'reference/qdos386/r5-resource-expansion-internal/volumes/win1-dev-r5-64.img'),'--output','build/alpha-reproduced.img']]
for i,cmd in enumerate(commands):
 with (dest/('fresh-build-'+str(i)+'.log')).open('w') as f:subprocess.run(cmd,cwd=fresh,stdout=f,stderr=subprocess.STDOUT,check=True)
assert sha(fresh/'build/alpha-reproduced.img')==expected,'Fresh source must reproduce exact A image'
outputs={}
for local,frozen in [('source/build/apollo-r6/pc-vga/kernel.aout','kernel.aout'),('source/build/apollo-r6/pc-vga/qdos386-vga.img','kernel-package.img'),('source/build/apollo-r6-shell/shell.aout','shell.aout'),('source/build/apollo-r6-shell/sh.aout','sh.aout')]:
 assert sha(fresh/local)==sha(stage/frozen),local;outputs[local]=sha(fresh/local)
for name in ['mbr.bin','vbr.bin','stage2.bin']:
 assert sha(fresh/'build/alpha-boot'/name)==sha(stage/('boot-'+name));outputs['boot/'+name]=sha(fresh/'build/alpha-boot'/name)
smoke=r.parent/'test-runs/qdos386/r6-alpha-fresh-4';cmd=['python3',str(fresh/'scripts/investigate-memory.py'),str(fresh/'build/alpha-reproduced.img'),str(fresh/'build/alpha-reproduced.img'),str(smoke),'--single-disk','--aout',str(fresh/'source/build/apollo-r6/pc-vga/kernel.aout'),'--smp','4','--memory','32','--scenario','A','--layout',str(fresh/'reports/r5/evidence/layout.json'),'--key-delay','.1','--timeout','150']
with (dest/'fresh-smoke.log').open('w') as f:subprocess.run(cmd,cwd=fresh,stdout=f,stderr=subprocess.STDOUT,check=True)
d=json.loads((smoke/'result.json').read_text());t=(smoke/'transcript.txt').read_text();assert d['exit_status']==0 and not d.get('error') and all(d['checks'].values()) and d['physical_disks']==d['command'].count('-drive')==1 and d['reference_unchanged']
assert all('GCC: '+s+' ' in t for s in ['CCCP','CC1','AS','LD']) and 'FIX1 guest GCC works!\nexit status 0' in t and 'IOW0' in t and 'PGFAULT:' not in t
verification=dict(status='PASS A-only fresh source',source_commit=source,accepted_kernel_tag='r6a-fat16-single-disk-accepted',accepted_kernel_source='1bd468eaa040a40f0b5bc7fd09ebe5d098b3bbd3',archive_sha256=sha(archive),tracked_files=len(files),fresh_commands=commands,fresh_image_sha256=expected,outputs=outputs,smoke=dict(command=d['command'],checks=d['checks'],physical_disks=1,exit_status=0,result_sha256=sha(smoke/'result.json'),transcript_sha256=sha(smoke/'transcript.txt'),reference_unchanged=True),fat32_guest='FAIL B1; excluded from candidate code',source_tree_post_build_unchanged=all(sha(fresh/p)==h for p,h in files.items()))
assert verification['source_tree_post_build_unchanged'];write(dest/'FRESH-VERIFICATION.json',verification)
provenance=dict(private_internal=True,stage='A-only private alpha candidate; FAT32 B1 FAILED',source_commit=source,kernel_source=verification['accepted_kernel_source'],tag=verification['accepted_kernel_tag'],image_sha256=expected,image_bytes=image.stat().st_size,partition=dict(type='FAT16',index=1,active=True,start=2048,sectors=65536),physical_disks=1,build_manifest='STAGE-A-IMAGE-MANIFEST.json',fresh_verification='FRESH-VERIFICATION.json',r5_reference_archive_sha256='06c64ef08f61527cabb8243eefcba82bf32b0b06f46256bfc1dcbb0db1cc56db',b_counterexample_source='626c161acbfafae8d16a5d29ef598ba78061b15b',b_counterexample_directory=str(ref/'stage-b'),raw_evidence=str(r.parent/'test-runs/qdos386/r6-*'),public_release=False,rights_review='Public allowlist empty; private sources/payloads preserved; see separate inventory',later_receipt='Documentation-only receipt may follow archived source; kernel/build sources unchanged')
write(dest/'PROVENANCE.json',provenance)
# Conservative review inventory: no file is granted public approval by its presence.
review=[]
for p,h in files.items():review.append(dict(file='source/'+p,sha256=h,source='tracked private repository at'+source,ownership='Project/historical provenance retained in source; not individually cleared',licensing_status='UNREVIEWED',approval=False,include_in_public_candidate=False))
payload=json.loads((r/'reports/r6/evidence/stage-a-payload.json').read_text())
for p,h in payload['payloads'].items():review.append(dict(file='volume'+p,sha256=h,source='Accepted private R5 fixture' if p!='/QDOS.SYS' else 'Derived accepted A kernel/transfer package',ownership='Existing recovered/ported application or project derivative',licensing_status='UNKNOWN / review original provenance',approval=False,include_in_public_candidate=False))
for p in sorted(dest.iterdir()):
 if p.is_file():review.append(dict(file='artifact/'+p.name,sha256=sha(p),source='Private candidate generated from pinned sources',ownership='Project derivative; image/source archive includes uncleared historical material',licensing_status='PENDING APPROVAL',approval=False,include_in_public_candidate=False))
write(dest/'PUBLIC-FILE-RIGHTS-INVENTORY.json',dict(status='REVIEW ONLY; NO PUBLIC EXPORT',files=review,policy='Unknown rights excluded only from public allowlist; no private file removed or modified'))
write(dest/'PUBLIC-CANDIDATE-MANIFEST.json',dict(status='NOT AUTHORIZED / EMPTY ALLOWLIST',approved_files=[],review_records=len(review),unknown_or_pending_excluded=len(review),private_laboratory_preserved=True,published=False))
checks={str(p.relative_to(dest)):sha(p) for p in sorted(dest.rglob('*')) if p.is_file()};(dest/'SHA256SUMS').write_text(''.join(h+'  '+p+'\n' for p,h in checks.items()));assert all(sha(dest/p)==h for p,h in checks.items())
print(json.dumps(dict(status='PASS A-only; B FAILED honestly preserved',source_commit=source,image_sha256=expected,archive_sha256=sha(archive),checksum_manifest_sha256=sha(dest/'SHA256SUMS'),artifacts=len(checks),public_approved_files=0),indent=2))
