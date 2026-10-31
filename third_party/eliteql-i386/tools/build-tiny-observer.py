#!/usr/bin/env python3
"""Build a test-only observer of the supplied emulator; never edit the OS.
Adds held PS/2 input and read-only RAM/framebuffer dumps, not guest shortcuts.
"""
from pathlib import Path
import sys,shutil,subprocess
root=Path(__file__).resolve().parent.parent;osroot=Path(sys.argv[1]).resolve();d=root/'build/tiny-observer'
if not d.exists():shutil.copytree(osroot/'emulator/src',d)
p=d/'main.c';s=(osroot/'emulator/src/main.c').read_text()
s=s.replace('int main(int argc, char *argv[])','''static void observer(PC *pc, void *fb, unsigned bytes) {
 static unsigned checks; static long pos;
 if (++checks%10) return;
 FILE *f=fopen("observer.commands","r"); if(!f)return;
 fseek(f,pos,SEEK_SET); char line[512],path[256]; unsigned a,n; int down;
 while(fgets(line,sizeof(line),f)) {
  if(sscanf(line,"key %x %d",&a,&down)==2)ps2_put_keycode(pc->kbd,down,a);
  else if(sscanf(line,"ram %255s %x %x",path,&a,&n)==3 && a+n<=(unsigned)pc->phys_mem_size){FILE *o=fopen(path,"wb");if(o){fwrite(pc->phys_mem+a,1,n,o);fclose(o);}}
  else if(sscanf(line,"frame %255s",path)==1){FILE *o=fopen(path,"wb");if(o){fwrite(fb,1,bytes,o);fclose(o);}}
 }
 pos=ftell(f);fclose(f);
}
int main(int argc, char *argv[])''')
s=s.replace('pc_vga_step(pc);','pc_vga_step(pc);\n        observer(pc,fb,conf.width*conf.height*4);')
p.write_text(s)
subprocess.run(['gcc','-O2','-DI386_ENABLE_FPU',*[str(p) for p in d.glob('*.c')],'-o',str(root/'build/tools/tiny-observer'),'-lm'],check=True)
