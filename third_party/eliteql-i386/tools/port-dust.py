#!/usr/bin/env python3
"""Allocate the RC6 static dust routine's byte temporaries to i386 registers.
This produces straight-line native assembly and native calls/branches. There
is no emulated CPU, dispatch loop, guest instruction stream or runtime wrapper.
The input itself is the shipped generated JAS module, not another Elite tree.
Reject any instruction or addressing form outside that source's fixed subset.
"""
from pathlib import Path
import re
root=Path(__file__).resolve().parent.parent
reg={'d0':'%al','d1':'%bl','d2':'%cl','d3':'%dl','d4':'%dh','d5':'%ah'}
out=['# Native i386 byte-register allocation of reference/src/dust_generated.s.', '# Reproduce with tools/port-dust.py. ESI=state, EDI=screen, EBP=index scratch.', '.text']
def operand(s):
 if s in reg:return reg[s]
 if s.startswith('#'):return '$'+s[1:]
 m=re.fullmatch(r'(\d+)\(a2(?:,(d[12])\.w)?\)',s)
 if m:
  if m[2]:out.append(' movzbl '+reg[m[2]]+',%ebp')
  return m[1]+'(%esi'+(',%ebp' if m[2] else '')+')'
 raise ValueError(s)
ops={'move.b':'movb','and.b':'andb','lsr.b':'shrb','eor.b':'xorb','roxr.b':'rcrb','addx.b':'adcb','or.b':'orb','subq.b':'subb','subx.b':'sbbb','sub.b':'subb','roxl.b':'rclb','lsl.b':'shlb','moveq':'movb'}
branches={'bra':'jmp','bsr':'call','bne':'jne','beq':'je','bpl':'jns','bmi':'js'}
for raw in (root/'reference/src/dust_generated.s').read_text().splitlines():
 line=raw.split(';')[0].strip()
 if not line:continue
 if line.startswith('.sect'):continue
 if line.startswith('.globl') or line.endswith(':'):out.append(line);continue
 op,*rest=line.split(None,1);arg=rest[0] if rest else ''
 if op in ops:
  args=re.split(r',\s*(?![^()]*\))',arg);assert len(args)==2
  a,b=[operand(x) for x in args];out.append(' '+ops[op]+' '+a+','+b)
 elif op in branches:out.append(' '+branches[op]+' '+arg)
 elif op=='tst.b':a=operand(arg);out.append(' testb '+a+','+a)
 elif op in ('scs','scc'):out.append(' '+('setb' if op=='scs' else 'setae')+' '+operand(arg))
 elif op=='rts':out.append(' ret')
 else:raise ValueError(line)
(root/'src/dust_generated.s').write_text('\n'.join(out)+'\n')
