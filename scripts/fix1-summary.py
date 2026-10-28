#!/usr/bin/env python3
"""R11M-FIX1: one-page summary of logs/fix1 (matrix judge, window smoke, demo, regression, forced cases)."""
import os, re, sys
D = sys.argv[1]


def rd(p):
    p = os.path.join(D, p)
    return open(p, errors="replace").read() if os.path.exists(p) else ""


out = []
mx = rd("matrix/judge.txt")
n_ok = len(re.findall(r"^  ok ", mx, re.M)); n_bad = len(re.findall(r"^  FAIL ", mx, re.M))
out.append("Tiny386 full-machine headless, patched emulator, 3 rounds x 4 boots, each boot a fresh process ended by the guest: %d ok, %d FAIL" % (n_ok, n_bad))
for l in mx.splitlines():
    if l.startswith("ROUND") or l.startswith("FIX1 TINY386"):
        out.append("  " + l)
for l in mx.splitlines():
    if l.strip().startswith("info"):
        out.append("  " + l.strip())
out.append("")
w = rd("window.log")
out.append("Windowed Tiny386 (rawdraw) under Xvfb: " + (re.findall(r"FIX1 WINDOW SMOKE \w+.*", w) or ["MISSING"])[0])
d = rd("demo.log")
out.append("R11M demonstration on patched Tiny386, both phases ended by the guest: " + (re.findall(r"FIX1 DEMO pc \w+", d) or ["MISSING"])[0])
r = rd("regress/filter.txt")
out.append("Regression (earlier rounds' sequences on the R11M kernels, QEMU + Tiny386, no i386VM; \"FAIL\" counts the checks of groups NOT RUN here): " + " | ".join(l for l in r.splitlines() if l.startswith(("PASS", "real failures"))))
q = rd("qemu-control-smoke.log")
out.append("QEMU control smoke: " + (re.findall(r"(?:FIX1 )?SMOKE (?:PASS|FAIL).*", q) or ["MISSING"])[0])
q2 = rd("qemu-port-8900/q1/qemu-q1.txt") or rd("qemu-port-8900/qemu-q1.txt")
out.append("QEMU 8.2.2 and the 0x8900 string: " + ("helper ran, 'SHUTDOWN: no response' (QEMU does not act on it)" if "SHUTDOWN: no response" in q2 else "not recorded"))
print("\n".join(out))
