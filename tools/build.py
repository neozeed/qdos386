#!/usr/bin/env python3
"""The single R7B2 build. Source is read only; every product lives in a marked tree."""
import argparse,fcntl,hashlib,importlib.util,json,os,shutil,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
R=Path(__file__).resolve().parents[1];S=R/'source'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

class Build:
 def __init__(self,a):
  self.a=a;self.out=a.output.absolute();o=self.out
  if o!=o.resolve() or o in [R,*R.parents,Path('/tmp')] or (R in o.parents and R/'build' not in o.parents):raise ValueError('Output must be a new dedicated directory, outside source or below root build/')
  self.marker=o/'.qdos386-build.json';identity=dict(schema=1,source=str(R))
  if o.exists():
   if not self.marker.is_file() or json.loads(self.marker.read_text())!=identity:raise ValueError('Refusing unmarked or foreign output directory: '+str(o))
  elif a.target=='clean':return
  else:o.mkdir(parents=True);self.marker.write_text(json.dumps(identity)+'\n')
  self.lock=(o/'.lock').open('w');fcntl.flock(self.lock,fcntl.LOCK_EX)
  if a.target=='clean':return
  for d in ['logs','tmp','products','objects','sdk/lib']: (o/d).mkdir(parents=True,exist_ok=True)
  self.env=dict(os.environ,TMPDIR=str(o/'tmp'),LC_ALL='C',SOURCE_DATE_EPOCH='1794700800',QDOS386_TOOLS=str(o/'toolchain'),QDOS386_LIBDIR=str(o/'sdk/lib'),QDOS386_SDK=str(S/'sdk'))
  self.statefile=o/'state.json';self.state=json.loads(self.statefile.read_text()) if self.statefile.exists() else {}
  inputs={}
  for base in [S/'kernel',S/'platform',S/'sdk',S/'port/include',S/'apps/common',S/'apps/hello',S/'apps/prep',S/'apps/shell',S/'apps/disktools',S/'mk',S/'tools',R/'toolchain',R/'tools',R/'validation/r7']:
   for p in sorted(base.rglob('*')):
    if p.is_file() and not any(n in ('__pycache__','build') for n in p.parts) and p.suffix not in ('.o','.a','.aout','.pyc'):inputs[str(p.relative_to(R))]=sha(p)
  self.fingerprint=hashlib.sha256(json.dumps([inputs,subprocess.check_output([a.hostcc,'--version'],text=True)],sort_keys=True).encode()).hexdigest()
  self.config=json.loads((S/'mk/current.json').read_text());self.gcc=o/'toolchain/bin/i386-gcc';self.asm=o/'toolchain/bin/as';self.ld=o/'toolchain/bin/i386-ld';self.log=None
 def run(self,args,cwd=None):
  args=list(map(str,args));self.log.write(json.dumps(args)+'\n');self.log.flush()
  p=subprocess.run(args,cwd=cwd or S,env=self.env,stdout=self.log,stderr=subprocess.STDOUT)
  if p.returncode:raise RuntimeError('Command failed (%d): %s; see %s'%(p.returncode,args,self.log.name))
 def stage(self,name,fn):
  key=self.fingerprint
  if name=='image':key+=sha(self.a.payload) if self.a.payload else 'source-only'
  old=self.state.get(name,{})
  if old.get('key')==key and all((self.out/p).is_file() and sha(self.out/p)==h for p,h in old.get('outputs',{}).items()):print(name+': current',flush=True);return
  print(name+': building',flush=True)
  with (self.out/'logs'/(name+'.log')).open('w') as self.log:
   products=fn()
  self.state[name]=dict(key=key,outputs={str(p.relative_to(self.out)):sha(p) for p in products});self.statefile.write_text(json.dumps(self.state,indent=2)+'\n')
 def toolchain(self):
  module('hostbuild',R/'tools/build-toolchain.py').build(self.out/'toolchain',self.a.hostcc)
  return list((self.out/'toolchain/bin').iterdir())
 def compile(self,src,out,flags=()):
  out.parent.mkdir(parents=True,exist_ok=True)
  if src.suffix.lower()=='.s':self.run([self.asm,'-o',out,src])
  else:self.run([self.gcc,'-c',*flags,src,'-o',out])
  return out
 def archive(self,name,objects):
  p=self.out/'sdk/lib'/name
  if p.exists():p.unlink()
  self.run(['ar','rcD',p,*objects]);return p
 def sdk(self):
  outputs=[]
  groups=[('libqdos.a','lib','qdos qtrap qdospg qdosfs qdoscon qdosjob qdosproc qdosenv qdostitle progname qdossurf qdosgfx qdosser'.split()),('libc.a','libc','string malloc stdlib stdio ctype errno strtod fpbig misc time scanf posixio jmp envx'.split()),('libm.a','libm','fabs modf floor ceil frexp ldexp sqrt pow xmath trans xtrans'.split())]
  for archive,folder,names in groups:
   objs=[]
   for n in names:
    src=S/'sdk'/folder/(n+'.c')
    if not src.exists():src=src.with_suffix('.s')
    objs.append(self.compile(src,self.out/'objects/sdk'/folder/(n+'.o'),['-O','-I'+str(S/'sdk/include'),'-I'+str(S/'port/include')]))
   outputs.append(self.archive(archive,objs))
  objs=[]
  for n in 'fixdfdi floatdidf fixunsdfdi negdi2 adddi3 subdi3 lshldi3 ashrdi3 cmpdi2 fixunsdfsi'.split():
   src=S/'sdk/compiler'/('gnulib.c' if n=='fixunsdfsi' else 'gnulib2.c')
   objs.append(self.compile(src,self.out/'objects/sdk/compiler'/(n+'.o'),['-O','-DL_'+n,'-I'+str(S/'sdk/compiler'),'-I'+str(S/'sdk/include')]))
  objs.append(self.compile(S/'sdk/compiler/fixdfsi.s',self.out/'objects/sdk/compiler/fixdfsi.o'));outputs.append(self.archive('libgcc-qd.a',objs))
  objs=[]
  for n,define in [('stdio','QDOS_C68_CONWRITE'),('posixio','QDOS_C68_POSIX')]:objs.append(self.compile(S/'sdk/libc'/(n+'.c'),self.out/'objects/sdk/libc'/(n+'_c68.o'),['-O','-D'+define,'-I'+str(S/'sdk/include'),'-I'+str(S/'port/include')]))
  outputs.append(self.archive('libc68.a',objs))
  outputs.append(self.compile(S/'sdk/lib/crt0_libc.s',self.out/'sdk/lib/crt0.o'))
  outputs.append(self.compile(S/'sdk/lib/crt0.s',self.out/'sdk/lib/crt0_minimal.o'));return outputs
 def shell(self):
  o=self.out;products=[];start=self.compile(S/'apps/common/app_start.s',o/'objects/apps/app_start.o');trap=self.compile(S/'apps/common/qtrap.s',o/'objects/apps/qtrap.o')
  for name,slot,extra in [('shell',0,[]),('sh',1,['-DR13B_DYN'])]:
   obj=self.compile(S/'apps/shell/shell.c',o/'objects/apps'/(name+'.o'),['-O',*self.config['shell_flags'],*extra,'-Iapps/common','-Iapps/shell'])
   dest=o/'products'/(name+'.aout');self.run([self.ld,'-N','-T',S/'apps/common'/('slot%d.ld'%slot),'-e','_start','-o',dest,start,obj,trap]);products.append(dest)
  return products
 def kernel(self):
  o=self.out;products=[];objs=o/'objects';boot=objs/'boot';boot.mkdir(parents=True,exist_ok=True)
  self.compile(S/'platform/pc/boot3.s',boot/'boot3.o');self.run([self.ld,'-N','-T',S/'platform/pc/boot.ld','-e','start','-o',boot/'boot3.aout',boot/'boot3.o']);self.run([sys.executable,R/'tools/aout-to-raw.py',boot/'boot3.aout',o/'products/boot3.bin']);products.append(o/'products/boot3.bin')
  # Preserve the accepted transfer directory and object/link order exactly.
  start=self.compile(S/'apps/common/app_start.s',objs/'apps/app_start.o');trap=self.compile(S/'apps/common/qtrap.s',objs/'apps/qtrap.o');xfer=[]
  for name,slot in [('hello',0),('prep',1)]:
   obj=self.compile(S/'apps'/name/(name+'.c'),objs/'apps'/(name+'.o'),['-Iapps/common']);dest=o/'products'/(name+'.aout');self.run([self.ld,'-N','-T',S/'apps/common'/('slot%d.ld'%slot),'-e','_start','-o',dest,start,obj,trap]);xfer.append(dest)
  self.run([sys.executable,S/'tools/make-xfer.py',o/'products/xfer.bin','HELO='+str(xfer[0]),'PREP='+str(xfer[1]),'HEL2='+str(xfer[0])]);products.append(o/'products/xfer.bin')
  for variant in ['serial','vga']:
   flags=['-I'+str(S/'kernel'),'-I'+str(S/'platform')]+(['-DQDOS_CON_VGA'] if variant=='vga' else [])+self.config['kernel_flags'];objects=[self.compile(S/'platform/pc/entry.s',objs/variant/'entry.o')]
   for name in self.config['kernel_objects']:
    candidates=[S/'kernel'/(name+'.c'),S/'kernel'/(name+'.s'),S/'platform/pc'/(name+'.c'),S/'platform/pc'/(name+'.s')];src=next(p for p in candidates if p.exists());objects.append(self.compile(src,objs/variant/(name+'.o'),flags))
   dest=o/'products'/('kernel-'+variant+'.aout');self.run([self.ld,'-N','-T',S/'platform/pc/kernel.ld','-e','kstart','-o',dest,*objects]);products.append(dest)
   raw=o/'products'/('kernel-'+variant+'.raw');self.run([sys.executable,S/'platform/pc/extract-kernel.py',dest,raw,'0x100000']);products.append(raw)
   pkg=o/'products'/('qdos386-'+variant+'.img');self.run([sys.executable,S/'platform/pc/make-image3.py',o/'products/boot3.bin',raw,o/'products/xfer.bin',pkg]);products.append(pkg)
  apollo=o/'products/apollo-boot'
  # This accepted BIOS-only builder already uses system binutils; kernel/SDK use historical tools.
  self.run([sys.executable,R/'tools/apollo/build-boot.py',apollo,'--rebuild'])
  products.extend(apollo.glob('*.bin'));return products
 def applications(self):
  products=[];o=self.out
  for n,mode in [('FDISK',0),('FORMAT',1),('CHKDSK',2)]:
   p=o/'products'/(n+'.aout');self.run([S/'sdk/bin/qdos386-gcc','-O','-I'+str(S/'kernel'),'-DTOOL_MODE='+str(mode),'-o',p,S/'apps/disktools/main.c',S/'kernel/fatmeta.c']);products.append(p)
  for n,src in [('F32IO','fat32-native-channels.c'),('F32LIVE','fat32-native-live.c')]:
   p=o/'products'/(n+'.AOU');self.run([S/'sdk/bin/qdos386-gcc','-O','-DQDOS_R20','-I'+str(S/'kernel'),'-o',p,R/'validation/r7'/src]);products.append(p)
  p=o/'products/populate';self.run([self.a.hostcc,'-std=c89','-Wall','-Wextra','-Werror','-DQDOS_FS_WRITE','-DQDOS_VFAT','-DQDOS_APOLLO_FAT32','-DQDOS_APOLLO_FAT32_MUTATION','-I'+str(S/'kernel'),R/'tools/populate-volume.c',S/'kernel/fat16.c','-o',p]);products.append(p);return products
 def image(self):
  return module('diskbuild',R/'tools/build-system-image.py').build(self)
 def clean(self):
  if self.out.exists():
   if R in self.out.parents and (R/'.git').exists():
    tracked=subprocess.check_output(['git','-C',str(R),'ls-files','--',str(self.out.relative_to(R))],text=True)
    if tracked:raise ValueError('Refusing to remove tracked files from the build tree')
   shutil.rmtree(self.out)
  print('Removed only designated build tree:',self.out)
 def execute(self):
  if self.a.target=='clean':self.clean();return
  order=[('toolchain',self.toolchain),('sdk',self.sdk),('shell',self.shell),('kernel',self.kernel),('disk-tools',self.applications),('image',self.image)]
  for name,fn in order:
   self.stage(name,fn)
   if name==self.a.target:break

def main():
 ap=argparse.ArgumentParser();ap.add_argument('target',choices=['toolchain','sdk','shell','kernel','disk-tools','image','clean']);ap.add_argument('--output',required=True,type=Path);ap.add_argument('--hostcc',default='gcc');ap.add_argument('--payload',type=Path);a=ap.parse_args();Build(a).execute()
if __name__=='__main__':main()
