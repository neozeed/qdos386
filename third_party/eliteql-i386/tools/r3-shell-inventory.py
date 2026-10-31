#!/usr/bin/env python3
"""Source coverage records translation ownership separately from test evidence."""
from pathlib import Path
import json,re,hashlib
r=Path(__file__).resolve().parent.parent
base=json.loads((r/'docs/R2-SOURCE-INVENTORY.json').read_text());shell=next(x for x in base if x['file']=='shell.s')
source=(r/'reference/src/shell.s').read_bytes();lines=source.decode().splitlines();assert hashlib.sha256(source).hexdigest()==shell['sha256']
owner={}
for line in (r/'src/shell_flow.s').read_text().splitlines():
 m=re.match(r'# shell.s:(\d+)',line)
 if m:owner[int(m[1])]='src/shell_flow.s'
# Every handwritten block below is a translation of the indicated original
# region, including its original internal branch labels (native labels can
# differ with register allocation). Data is inventoried separately.
blocks=[('asm_clock','asm_setup','shell_start'),('asm_setup','asm_abort_target','shell_setup'),('asm_abort_target','asm_contact','shell_spawn'),('asm_spawn_missile','asm_draw_dials','shell_spawn'),('asm_draw_dials','asm_fps','shell_scene'),('asm_clear_view','asm_decimal','shell_world'),('asm_decimal','asm_commander_page','shell_text'),('asm_labels','asm_navigation_keys','shell_text'),('save_copy','save_parts','shell_save'),('asm_encounters','asm_farof','shell_world'),('asm_warp','asm_warp_locked_text','shell_world'),('asm_view_label','asm_station_ai','shell_scene'),('asm_station_ai','asm_current_laser','shell_world'),('asm_dust_run','asm_chart_from_target','shell_world'),('asm_sound_event','release_commander','shell_sound'),('uni_allocate','cheat_active','shell_universe'),('cheat_actions','menu_escape','shell_cheats')]
for a,b,f in blocks:
 start=next(i+1 for i,l in enumerate(lines) if l.startswith(a+':'));end=next(i+1 for i,l in enumerate(lines) if l.startswith(b+':'))
 for n in range(start,end):owner.setdefault(n,'src/'+f+'.s')
# Wrappers and source blocks that share code regions have explicit ownership.
for name,file in [('asm_clear_view','shell_start'),('asm_clear_view_loop','shell_start'),('asm_dust_run','shell_scene'),('uni_spawn_allocate','shell_spawn'),('uni_encounter_allocate','shell_world')]:
 owner[next(i+1 for i,l in enumerate(lines) if l.startswith(name+':'))]='src/'+file+'.s'
data=json.loads((r/'docs/R3-SHELL-DATA.json').read_text())
# Label ownership is discovered from the actual native definitions. This
# does not imply that mere label existence proves the procedure's behavior.
native={}
for p in (r/'src').glob('shell_*.s'):
 for m in re.finditer(r'(?m)^([A-Za-z_]\w*):',p.read_text()):native[m[1]]=str(p.relative_to(r))
missing=[]
for item in shell['items']:
 n=item['line'];name=item['name'];p=native.get(name) or owner.get(n)
 if not p:
  # A generated flow label immediately precedes its first translated instruction.
  for j in range(n+1,min(n+12,len(lines))):
   if re.match(r'^\w+:',lines[j-1]):break
   if j in owner:p=owner[j];break
 item['implementation']=p;item['status']='translated; subsystem tests listed separately' if p else 'unmapped'
 if not p:missing.append((name,n))
uncovered=[]
for n,line in enumerate(lines,1):
 code=line.split(';')[0].strip()
 if code and not code.startswith('.') and ':' not in code and n not in owner:uncovered.append((n,code))
shell['instruction_lines_without_mapping']=uncovered
shell['test_evidence']={'setup':'tools/diff-shell-setup.py','loop':'tools/diff-shell-loop.py','lifecycle':'tools/diff-shell-lifecycle.py','universe':'tools/diff-shell-universe.py','serialization':'tools/diff-shell-serialization.py','world':'tools/diff-shell-world.py','pages':'tools/diff-shell-pages.py','platform':'tools/test-qdos-adapter.py','os_session':'tools/run-tiny-session.py','text_save_leaves':'tools/diff-shell-text.py','core':'frozen R2 fixture/platform/oracle targets'}
shell['functional_completeness']='Translation coverage is not a functional proof. OS session and additional path tests are reported separately.'
(r/'docs/R3-SHELL-INVENTORY.json').write_text(json.dumps(shell,indent=2)+'\n')
print(len(shell['items']),'labels;',len(missing),'unmapped labels;',len(uncovered),'unmapped instruction lines')
print(missing);print(uncovered)
assert not missing and not uncovered
