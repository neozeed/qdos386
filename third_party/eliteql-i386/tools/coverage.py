#!/usr/bin/env python3
"""Report exported-symbol translation coverage, not a proof of fidelity."""
from pathlib import Path
import re,hashlib
root=Path(__file__).resolve().parent.parent
exports=lambda p: set(re.findall(r'^\s*\.globl\s+(\w+)',p.read_text(),re.M))
native=set().union(*(exports(p) for p in (root/'src').glob('*.s')))
lines=['# RC6 assembly coverage — R1','',
 'Coverage means that a native definition exists. Passing tests are recorded separately.',
 'Data symbols and internal helpers are counted too; this is not a feature-completion percentage.','',
 '| Original source | Exports present | Remaining exported symbols |',
 '|---|---:|---|']
for p in sorted((root/'reference/src').glob('*.s')):
 ex=exports(p);missing=sorted(ex-native)
 lines.append('| '+p.name+' | '+str(len(ex&native))+'/'+str(len(ex))+' | '+(', '.join('`'+s+'`' for s in missing) or '—')+' |')
lines+=['','Files with no exports may contain required local code or tables; 0/0 does not mean ported.',
 'The two render/model variants and release/development variants remain separate upstream choices.',
 'The shell and hardware-facing routines have not been stubbed.','',
 '## Native definitions','']+['- `'+s+'`' for s in sorted(native)]
(root/'docs/COVERAGE.md').write_text('\n'.join(lines)+'\n')
manifest=[]
for p in sorted((root/'reference').rglob('*')):
 if p.is_file():manifest.append(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(root)))
(root/'docs/REFERENCE-SHA256SUMS').write_text('\n'.join(manifest)+'\n')
print(str(len(native))+' native exported definitions; coverage and reference manifest written')
