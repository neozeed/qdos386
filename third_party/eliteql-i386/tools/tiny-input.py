#!/usr/bin/env python3
"""Script held PS/2 keys through the test emulator's hardware input observer."""
from pathlib import Path
import sys,time
p=Path(__file__).resolve().parent.parent/'build/os-session/observer.commands'
scan={c:n for c,n in zip('1234567890',range(2,12))};scan.update({c:n for c,n in zip('qwertyuiop',range(16,26))});scan.update({c:n for c,n in zip('asdfghjkl',range(30,39))});scan.update({c:n for c,n in zip('zxcvbnm',range(44,51))});scan.update({' ':57,'.':52,',':51,'/':53,'\\':43,'\n':28,'-':12,'\b':14,'\x1b':1})
with p.open('a',buffering=1) as f:
 for c in sys.argv[1].lower():
  n=scan[c];f.write(f'key {n:x} 1\n');time.sleep(float(sys.argv[2]) if len(sys.argv)>2 else .12);f.write(f'key {n:x} 0\n');time.sleep(.12)
