#!/usr/bin/env python3
"""Exhaustive RC6 label/data/directive and call inventory; explicit coverage."""
from pathlib import Path
import re,json,hashlib
root=Path(__file__).resolve().parent.parent
native={};native_labels={}
for p in sorted((root/'src').glob('*.s')):
 text=p.read_text();rel=str(p.relative_to(root))
 for name in re.findall(r'^\s*\.globl\s+(\w+)',text,re.M):native.setdefault(name,[]).append(rel)
 native_labels[p.name]=set(re.findall(r'^\s*(\w+):',text,re.M))
rows=[]
for p in sorted((root/'reference/src').glob('*.s')):
 lines=p.read_text().splitlines();exports=set(re.findall(r'^\s*\.globl\s+(\w+)',p.read_text(),re.M));section='';items=[];refs=[];directives=[];owner=None
 module=str(Path('src')/p.name) if p.name in native_labels else None
 for n,line in enumerate(lines,1):
  code=line.split(';')[0].strip()
  if code.startswith('.sect'):section=code.split()[-1]
  match=re.match(r'([A-Za-z_]\w*):\s*(.*)',code)
  if match:
   name,tail=match.groups();owner=name;after=tail
   if not after:after=next((x.split(';')[0].strip() for x in lines[n:] if x.split(';')[0].strip()),'')
   kind='data' if after.startswith(('.data','.ascii','.space','.align')) or section in ('.data','.bss','.rom') else 'code'
   impl=native.get(name) if name in exports else None
   if not impl and module:impl=[module]
   scope='exported symbol' if name in exports and name in native else 'module translation' if impl else None
   status='translated' if impl else 'not yet translated'
   if p.name in ('save.s','keyboard.s') and kind=='code':status='platform-boundary'
   items.append(dict(name=name,line=n,kind=kind,section=section,exported=name in exports,status=status,implementation=impl,mapping_scope=scope))
  data=re.search(r'\.(?:data[124]|ascii|space|align)\b.*',code)
  if data:directives.append(dict(line=n,owner=owner,section=section,source=data.group()))
  if re.match(r'(?:\.extern|jsr|bsr|jmp|trap)\b',code):refs.append(dict(line=n,source=code))
 rows.append(dict(file=p.name,sha256=hashlib.sha256(p.read_bytes()).hexdigest(),implementation=module,items=items,data_directives=directives,references=refs))
# Resolve cross-file uses against every original definition, including data pointers.
definitions={}
for row in rows:
 for item in row['items']:definitions.setdefault(item['name'],set()).add(row['file'])
for row in rows:
 row['cross_file_references']=[];row['hardware_references']=[]
 for line_no,raw in enumerate((root/'reference/src'/row['file']).read_text().splitlines(),1):
  code=raw.split(';')[0]
  code=re.sub(r'"(?:[^"\\]|\\.)*"','',code)
  for symbol in set(re.findall(r'[A-Za-z_]\w*',code)):
   targets=sorted(definitions.get(symbol,set())-{row['file']})
   if targets:row['cross_file_references'].append(dict(line=line_no,symbol=symbol,defined_in=targets))
  if re.search(r'\btrap\b|0x20000|0xe2|__sys_var',code):
   row['hardware_references'].append(dict(line=line_no,source=code.strip()))
(root/'docs/R2-SOURCE-INVENTORY.json').write_text(json.dumps(rows,indent=2)+'\n')
out=['# R2 work-in-progress source coverage','',
'Every original label, including local branch labels and data objects, has an explicit status. JSON also records every data/alignment directive, its owning label, original SHA256, and external/call/trap references.',
'`translated` means source implementation exists; it does not mean independently verified. See R2-DIFFERENTIAL-TESTS.md. Local labels reorganized by native register allocation map to their complete module; this does not imply one-to-one instruction or address mapping.',
'**The original shell.s is not translated. R2 is not complete and is not a runnable game.**',
'Additional native random.s contains combat.s DORND; computer_dock.s contains docking.s MA64. Benchmark render.s/models.s are alternatives, not linked alongside game variants.','']
for f in rows:
 out += ['## '+f['file'],'','| Source label | Line | Kind | Status | Implementation |','|---|---:|---|---|---|']
 for i in f['items']:out.append('| `'+i['name']+'` | '+str(i['line'])+' | '+i['kind']+' | '+i['status']+' | '+(', '.join(i['implementation']) if i['implementation'] else '—')+' |')
 out+=['','External/call/OS references:']+['- Line '+str(x['line'])+': `'+x['source']+'`' for x in f['references']]+['']
(root/'docs/R2-SOURCE-COVERAGE.md').write_text('\n'.join(out)+'\n')
print(len(rows),'files;',sum(len(x['items']) for x in rows),'labels;',sum(bool(x['implementation']) for x in rows),'files with native counterparts;',len(native),'unique native exports')
