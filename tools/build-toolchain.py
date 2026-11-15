#!/usr/bin/env python3
"""Build project-owned historical host tools. No downloads or source rewriting."""
import hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1]

def build(out,hostcc='gcc'):
 out=Path(out).resolve();out.mkdir(parents=True,exist_ok=True)
 for d in ('bin','objects','logs','tmp'): (out/d).mkdir(exist_ok=True)
 env=dict(os.environ,TMPDIR=str(out/'tmp'),LC_ALL='C',SOURCE_DATE_EPOCH='1794700800')
 commands=[]
 def run(label,args):
  args=list(map(str,args));p=subprocess.run(args,cwd=out,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
  (out/'logs'/(label+'.log')).write_bytes(p.stdout);commands.append(dict(argv=args,exit=p.returncode));(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if p.returncode:raise RuntimeError(label+': '+p.stdout.decode(errors='replace')[-3000:])
 # Stabilize host ELF debugging metadata without touching historical source.
 common=['-std=gnu89','-w','-O0','-g','-ffile-prefix-map='+str(R)+'=/qdos386','-ffile-prefix-map='+str(out)+'=/qdos386-build/toolchain']
 s=R/'toolchain/host/gcc-1.40';g=R/'toolchain/generated'
 flags=common+['-DLEGACY_HOST64','-DTARGET_CPU_I386','-I'+str(g),'-I'+str(s),'-I'+str(s/'config')]
 names='c-decl c-typeck c-conv toplev version tree print-tree stor-layout fold-const rtl rtlanal expr stmt expmed explow optabs varasm symout dbxout sdbout emit-rtl integrate jump cse loop flow stupid combine regclass local-alloc global-alloc reload reload1 caller-save final recog pragma obstack'.split()
 generated='c-tab insn-emt insn-xrt insn-out insn-pep insn-rcg'.split()
 for n in names+generated:run('gcc-'+n,[hostcc,*flags,'-c',(g if n in generated else s)/(n+'.c'),'-o',out/'objects'/('gcc-'+n+'.o')])
 run('cc1-link',[hostcc,*[out/'objects'/('gcc-'+n+'.o') for n in sorted(names+generated)],'-lm','-o',out/'bin/cc1'])
 for n in ['cccp','cexp']:
  run('cpp-'+n,[hostcc,*flags,'-c',(g if n=='cexp' else s)/(n+'.c'),'-o',out/'objects'/('cpp-'+n+'.o')])
 run('cccp-link',[hostcc,out/'objects/cpp-cccp.o',out/'objects/cpp-cexp.o',out/'objects/gcc-version.o',out/'objects/gcc-obstack.o','-o',out/'bin/cccp'])
 gas=R/'toolchain/src-guest-staged/gas-1.38'
 names='as xrealloc xmalloc hash hex-value atof-generic append messages expr app frags input-file input-scrub output-file subsegs symbols version flonum-const flonum-copy flonum-mult bignum-copy obstack error i386 atof-ieee write read'.split()
 run('gas',[hostcc,*common,'-fcommon','-DLEGACY_HOST64','-DUSG','-DI386','-include',gas/'host64.h','-I'+str(gas),*[gas/(n+'.c') for n in names],'-o',out/'bin/as'])
 ld=R/'toolchain/src-guest-staged/binutils-1.94';cfg=R/'toolchain/host/ld-config'
 flags=common+['-fcommon','-Werror=implicit-function-declaration','-include',str(cfg/'sysdep.h'),'-I'+str(cfg),'-I'+str(ld/'include'),'-I'+str(ld/'bfd'),'-I'+str(ld/'ld')]
 bfd='libbfd opncls bfd archive targets cache archures core section format syms reloc init ctor aout32 i386aout cpu-i386 stab-syms'.split()
 for n in bfd:run('bfd-'+n,[hostcc,*flags,'-DMINIMIZE=1','-DDEFAULT_VECTOR=i386aout_vec','-DSELECT_ARCHITECTURES=bfd_i386_arch','-c',ld/'bfd'/(n+'.c'),'-o',out/'objects'/('bfd-'+n+'.o')])
 run('bfd-obstack',[hostcc,*flags,'-c',ld/'libiberty/obstack.c','-o',out/'objects/bfd-obstack.o'])
 names='ldgram ldlex ldlang ldctor ldmain ldindr ldwarn ldwrite ldexp ldemul ldver ldmisc ldsym ldvanilla ldfile i386'.split()
 for n in names:run('ld-'+n,[hostcc,*flags,'-c',ld/'ld'/(n+'.c'),'-o',out/'objects'/('ld-'+n+'.o')])
 run('ld-link',[hostcc,*[out/'objects'/('ld-'+n+'.o') for n in names],*[out/'objects'/('bfd-'+n+'.o') for n in bfd+['obstack']],'-o',out/'bin/i386-ld'])
 shutil.copy2(R/'toolchain/bin/i386-gcc',out/'bin/i386-gcc')
 hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((out/'bin').iterdir())}
 (out/'receipt.json').write_text(json.dumps(dict(host_compiler=subprocess.check_output([hostcc,'--version'],text=True).splitlines()[0],hashes=hashes),indent=2)+'\n')
 print('Built GCC 1.40 / GAS 1.38 / LD 1.94:',out,flush=True)
if __name__=='__main__':build(sys.argv[1],os.environ.get('HOSTCC','gcc'))
