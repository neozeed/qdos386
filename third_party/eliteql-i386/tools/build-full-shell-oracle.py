#!/usr/bin/env python3
"""Build all original production assembly as a separate behavioral oracle.
Reuses the frozen syntax-only converter, restoring its generated output.
__sys_var is a test fixture pointer; OS traps are intercepted by the harness.
"""
from pathlib import Path
import subprocess,sys
r=Path(__file__).resolve().parent.parent
p=r/'oracle/core-m68k.s';prior=p.read_bytes() if p.exists() else None
try:
 names=[p.stem for p in sorted((r/'reference/src').glob('*.s')) if p.stem not in ['development','render','models']]
 subprocess.run([sys.executable,str(r/'tools/build-m68k-oracle.py'),*names],check=True)
 (r/'oracle/full-shell-m68k.s').write_bytes(p.read_bytes()+b'\n.data\n__sys_var: .long 0\n')
 subprocess.run([str(r/'build/tools/as138-m68k'),'-m68000','-o',str(r/'oracle/full-shell-m68k.o'),str(r/'oracle/full-shell-m68k.s')],check=True)
finally:
 if prior is None:p.unlink(missing_ok=True)
 else:p.write_bytes(prior)
