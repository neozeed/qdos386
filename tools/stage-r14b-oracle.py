#!/usr/bin/env python3
"""Verify the supplied R14B archive; stage saved tool objects and binary oracles."""
import argparse, hashlib, json, tarfile
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('archive',type=Path);p.add_argument('stage',type=Path)
a=p.parse_args()
# Exact supplied archive identity is part of the restoration provenance.
expected=Path(__file__).resolve().parents[1]/'reports/r2a-disk-fix1/evidence/input-hashes.txt'
digest=expected.read_text().split()[0]
assert hashlib.sha256(a.archive.read_bytes()).hexdigest()==digest,'R14B archive hash differs'
with tarfile.open(a.archive) as archive:
 prefix='qdos386-R14B-GCC-ENV-SDK/'
 sums={name:h for line in archive.extractfile(prefix+'SHA256SUMS').read().decode().splitlines() for h,name in [line.split(None,1)]}
 rows=[]
 for m in archive:
  if not m.isfile():continue
  rel=m.name.removeprefix(prefix);dest=None
  if rel.startswith('source/toolchain/r11l/objects/') and rel.endswith('.o'):
   dest=a.stage/'toolchain/objects'/rel.split('/objects/',1)[1]
  elif rel=='source/toolchain/r11l/host-bin/cc1-lp64-repaired':dest=a.stage/'build/r14b/bootstrap/cc1'
  elif rel.startswith('guest-bin/') and rel.endswith('.AOU'):dest=a.stage/'build/r14b/oracle'/Path(rel).name
  if dest is None:continue
  data=archive.extractfile(m).read();actual=hashlib.sha256(data).hexdigest();assert actual==sums[rel]
  dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
  if dest.name=='cc1':dest.chmod(0o755)
  rows.append({'archive':rel,'staged':str(dest.relative_to(a.stage)),'sha256':actual})
 (a.stage/'build/r14b/oracle-manifest.json').write_text(json.dumps(rows,indent=2)+'\n')
 print('staged',len(rows),'verified object/oracle/bootstrap inputs')
