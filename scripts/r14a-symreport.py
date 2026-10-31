#!/usr/bin/env python3
"""R14A: external-symbol report for the native Hack 1.03 link (Gate 1).

Reads the Hack objects and the libraries in link order (GAS 1.38 a.out, which host nm cannot read) and prints, for
every external that a Hack object references but no Hack object defines, the first library member that defines it,
i.e. the member the linker takes.  Any reference left without a definition is listed as UNRESOLVED and makes the
script exit 1.  Output is sorted, so it is deterministic.
    usage: python3 scripts/r14a-symreport.py OBJDIR SHIM.o LIB.a [LIB.a ...]   (in link order)
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import importlib.util
spec = importlib.util.spec_from_file_location("syms", os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                                                   "r11d-symbols.py"))
syms = importlib.util.module_from_spec(spec); spec.loader.exec_module(syms)

def main(objdir, libs):
    objs = sorted(f for f in os.listdir(objdir) if f.endswith(".o") and f not in ("crt0.o", "c68vt.o"))
    objs = ["crt0.o"] + objs
    defined, refs = {}, {}
    for o in objs:
        data = open(os.path.join(objdir, o), "rb").read()
        for kind, name in syms.symbols(data):
            if kind == "U":
                refs.setdefault(name, set()).add(o)
            else:
                defined.setdefault(name, o)
    libdefs = {}
    for lib in libs:
        data = open(lib, "rb").read()
        mem = [(os.path.basename(lib), data)] if lib.endswith(".o") else syms.members(data)
        for mname, body in mem:
            for kind, name in syms.symbols(body):
                if kind not in ("U", "C"):
                    libdefs.setdefault(name, mname if lib.endswith(".o") else "%s(%s)" % (os.path.basename(lib), mname))
    bad = 0
    print("# R14A Hack 1.03: externals referenced by the Hack objects and not defined by them")
    print("# symbol  provider  referenced-by")
    for name in sorted(refs):
        if name in defined:
            continue
        prov = libdefs.get(name)
        if prov is None:
            prov = "UNRESOLVED"; bad += 1
        print("%-28s %-32s %s" % (name, prov, ",".join(sorted(refs[name]))))
    print("# objects: %d; externals from libraries: %d; unresolved: %d" %
          (len(objs), sum(1 for n in refs if n not in defined), bad))
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2:]))
