import subprocess,os,json,time,select,re,sys
from pathlib import Path
B=Path(os.environ["QDOS_R7_WORK"]).resolve()
assert str(B).startswith("/tmp/")
class Guest:
 def __init__(self,image,tag='gate0',extra=()):
  self.log=B/'logs'/tag; self.log.mkdir(exist_ok=False); self.shadow=None; self.prev=None; self.rows=[]
  self.err=(self.log/'stderr.txt').open('w'); self.trace=(self.log/'screens.txt').open('w')
  args=['qemu-system-i386','-machine','pc','-accel','tcg','-cpu','pentium','-m','32','-smp','4','-vga','std','-display','none','-serial','none','-nic','none','-qmp','stdio','-drive',f'file={image},format=raw,if=ide,index=0','-debugcon',f'file:{self.log}/debug.txt','-global','isa-debugcon.iobase=0x402','-boot','order=c','-no-reboot',*extra]
  (self.log/'command.json').write_text(json.dumps(args,indent=2)+'\n')
  self.p=subprocess.Popen(args,cwd=B,env=dict(os.environ,TMPDIR=str(B/'tmp')),stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=self.err,bufsize=0); self.buf=b''
  self.recv(); self.qmp('qmp_capabilities'); self.wait(lambda s:'Type HELP' in s,20); self.wait(self.prompt,10)
 def recv(self,timeout=5):
  end=time.monotonic()+timeout
  while b'\n' not in self.buf:
   if not select.select([self.p.stdout],[],[],max(0,end-time.monotonic()))[0]: raise TimeoutError('QMP response')
   x=os.read(self.p.stdout.fileno(),65536)
   if not x: raise RuntimeError('QEMU exited')
   self.buf+=x
  line,self.buf=self.buf.split(b'\n',1); return json.loads(line)
 def qmp(self,cmd,args=None):
  self.p.stdin.write((json.dumps({'execute':cmd,**({'arguments':args} if args else {})})+'\n').encode())
  while True:
   r=self.recv()
   if 'error' in r: raise RuntimeError(r)
   if 'return' in r: return r['return']
 def hmp(self,c): return self.qmp('human-monitor-command',{'command-line':c})
 def screen(self):
  debug=(self.log/'debug.txt').read_text(errors='replace') if (self.log/'debug.txt').exists() else ''
  if re.search(r'SYSTEM #|supervisor.*fault|PGFAULT|Supervisor #',debug,re.I): raise RuntimeError('Kernel fault: see debug.txt')
  if self.shadow is None:
   m=re.search(r'PRESENTER shadow 0x([0-9a-f]+)',debug)
   if m:self.shadow=int(m[1],16)
  addr=0xb8000
  if self.shadow:
   raw=self.hmp(f'xp /3xw {self.shadow:#x}'); words=re.findall(r'0x([0-9a-f]{8})',raw.split(':',1)[-1])
   if len(words)>2 and int(words[2],16)==3:addr=self.shadow+48
  raw=self.hmp(f'xp /4000xb {addr:#x}'); vals=[]
  for line in raw.splitlines():
   if ':' in line:vals.extend(int(x,16) for x in re.findall(r'0x([0-9a-f]{2})\b',line.split(':',1)[1]))
  c=bytes(vals[::2]); s='\n'.join(c[i:i+80].decode('cp437').rstrip() for i in range(0,2000,80))
  if s!=self.prev:self.trace.write('\n=== SCREEN %.3f\n%s\n'%(time.monotonic(),s));self.trace.flush();self.prev=s
  return s
 @staticmethod
 def prompt(s):return bool(re.search(r'^/[^\n>]*>\s*$',s.rstrip(),re.M)) and s.rstrip().splitlines()[-1].endswith('>')
 def wait(self,pred,seconds):
  end=time.monotonic()+seconds
  while time.monotonic()<end:
   s=self.screen()
   if pred(s):return s
   time.sleep(.08)
  raise TimeoutError('Guest wait: '+s)
 def key(self,k):self.hmp('sendkey '+k+' 30');time.sleep(.045)
 def type(self,s):
  keys={' ':'spc','.':'dot','/':'slash','-':'minus','_':'shift-minus',':':'shift-semicolon','=':'equal','\n':'ret','"':'shift-apostrophe'}
  for c in s:self.key('shift-'+c.lower() if c.isupper() else keys.get(c,c))
 def command(self,s,seconds=90):
  print('guest:',s,flush=True);self.trace.write('\n=== COMMAND '+s+'\n');self.trace.flush();self.type(s+'\n');time.sleep(.2);return self.wait(self.prompt,seconds)
 def close(self):
  try:self.qmp('quit')
  finally:
   try:self.p.wait(timeout=5)
   except subprocess.TimeoutExpired:self.p.kill();self.p.wait()
   self.trace.close();self.err.close()
