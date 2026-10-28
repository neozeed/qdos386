#!/usr/bin/env python3
"""R11L Phase 0: derive docs/R11L-DEPENDENCIES.csv from the real link inputs.  For each guest tool the objects it is built
from (build/r11l/{gas,gcc,ld}) are scanned; every symbol they reference that the SDK libraries define is listed with the
library and archive member that supplies it (the library the linker actually uses, in link order libm, libc, libgcc, libqdos).
usage: r11l-deps-csv.py > docs/R11L-DEPENDENCIES.csv   (run from the repository root after the tools were built)"""
import glob, os, sys
sys.argv = [sys.argv[0]]
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'r11l-undef.py')).read().replace('\nmain()\n', '\n')
ns = {}
exec(compile(src, 'r11l-undef', 'exec'), ns)
syms_of, members = ns['syms_of'], ns['members']

TOOLS = {
    'AS': sorted(f for f in glob.glob('build/r11l/gas/*.o') if not f.endswith('crt0.o')),
    'CCCP': ['build/r11l/gcc/cccp.o', 'build/r11l/gcc/cexp.o', 'build/r11l/gcc/version.o'],
    'CC1': sorted(f for f in glob.glob('build/r11l/gcc/*.o') if os.path.basename(f) not in ('cccp.o', 'cexp.o', 'crt0.o')),
    'LD': sorted(glob.glob('build/r11l/ld/*.o')),
}
LIBS = [('libm', 'sdk/lib/libm.a'), ('libc', 'sdk/lib/libc.a'), ('libgcc', 'sdk/lib/libgcc-qd.a'), ('libqdos', 'sdk/lib/libqdos.a')]
provider = {}
for lib, path in LIBS:
    for n, body in members(path):
        d, u = syms_of(body)
        for s in d:
            provider.setdefault(s, (lib, n.split('(')[-1].rstrip(')').rstrip('/')))
print('tool,symbol,library,member,referenced_by')
for tool, objs in TOOLS.items():
    defined, refs = set(), {}
    for o in objs:
        for n, body in members(o):
            d, u = syms_of(body)
            defined |= d
            for s in u:
                refs.setdefault(s, set()).add(os.path.basename(n))
    for s in sorted(refs):
        if s in defined or s not in provider:
            continue
        lib, mem = provider[s]
        print('%s,%s,%s,%s,%s' % (tool, s, lib, mem, ' '.join(sorted(refs[s]))[:60]))
