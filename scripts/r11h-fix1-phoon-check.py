#!/usr/bin/env python3
"""FIX1: check a Phoon transcript against tests/fix1/phoon-2106-expect.txt.

Usage: r11h-fix1-phoon-check.py TRANSCRIPT [EXPECT]
Pairs each "> RUN PHOON.AOU" line with the exit status that follows it (before the next RUN line)
and compares it with the expected status.  Exit 0 only if every expected case matches.
"""
import re, sys, os
here = os.path.dirname(os.path.abspath(__file__))
exp_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(here, '..', 'tests', 'fix1', 'phoon-2106-expect.txt')
runs, cur = [], None
for line in open(sys.argv[1], errors='replace'):
    line = line.rstrip('\n')
    m = re.match(r'^> RUN PHOON\.AOU "(.*)"$', line)
    if m:
        cur = {'arg': m.group(1), 'status': None}
        runs.append(cur)
        continue
    if cur is not None and cur['status'] is None:
        m = re.match(r'^exit status (\d+)$', line)
        if m:
            cur['status'] = int(m.group(1))
expect = []
for line in open(exp_path):
    if line.startswith('#') or not line.strip():
        continue
    idx, st, _ = line.split(None, 2) if len(line.split(None, 2)) == 3 else (line.split() + ['-'])[:3]
    expect.append((int(idx), int(st)))
inputs = [l.strip() for l in open(os.path.join(here, '..', 'tests', 'fix1', 'phoon-2106-input.txt')) if l.strip()]
bad = 0
for idx, st in expect:
    want_arg = re.match(r'^RUN PHOON\.AOU "(.*)"$', inputs[idx - 1]).group(1)
    got = [r for r in runs if r['arg'] == want_arg]
    got_st = got[0]['status'] if got else 'missing'
    ok = got_st == st
    bad += 0 if ok else 1
    print('%-4s %-22s want %s got %s' % ('ok' if ok else 'FAIL', want_arg, st, got_st))
print('PHOON 2106 CHECK: %s' % ('PASS' if bad == 0 else 'FAIL'))
sys.exit(0 if bad == 0 else 1)
