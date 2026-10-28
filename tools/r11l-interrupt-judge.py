#!/usr/bin/env python3
"""Judge one image left by an interrupted (or completed) operation of scripts/r11l-interrupt.sh.
usage: r11l-interrupt-judge.py OP N DONE IMG CSV PREFIX

Semantic classes (what the files contain, from the checker's extraction):
  before   as the base volume had it (operation had no visible effect)
  after    the operation's full result
  partial  a state between the two that the rules allow (e.g. a prefix of the new data; see below)
  broken   anything else: content that is neither old nor new nor an allowed intermediate
The judge reads the image with the checker's own parser, then repairs a COPY with --repair and
re-checks it with both the checker and fsck.fat.
"""
import os, shutil, subprocess, sys, tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import importlib.util
spec = importlib.util.spec_from_file_location('fatcheck', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'r11l-fatcheck.py'))
fc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fc)


def pat(i, seed):
    return (i * 131 + seed * 17 + (i >> 8) * 7) & 0xff


def pattern(n, seed, start=0):
    return bytes(pat(start + i, seed) for i in range(n))


def files_of(img):
    v = fc.Vol(img)
    errors, warns, files, orphans, stats = fc.analyse(v)
    out = {}
    for rec in files:
        if rec['attr'] & 0x10:
            continue
        try:
            out[rec['name']] = (fc.file_bytes(v, rec), rec)
        except Exception:
            out[rec['name']] = (None, rec)
    return v, errors, warns, out


def semantic(op, files):
    pre = pattern(3000, 31)
    got = files.get('PRE.DAT')
    new = files.get('NEW.DAT')
    moved = files.get('MOVED.DAT')
    pd = got[0] if got else None
    if op == 'create':
        if new is None:
            return 'before'
        d = new[0]
        if d == pattern(3000, 41):
            return 'after'
        if d is not None and d == pattern(len(d), 41):
            return 'partial'
        return 'broken'
    if op == 'append':
        if pd is None:
            return 'broken'
        if pd == pre:
            return 'before'
        full = pre + pattern(2000, 31, 3000)
        if pd == full:
            return 'after'
        if len(pd) >= 3000 and pd == pattern(len(pd), 31):
            return 'partial'
        return 'broken'
    if op == 'overwrite':
        if pd is None or len(pd) != 3000:
            return 'broken'
        newd = pre[:1000] + pattern(1000, 32, 1000) + pre[2000:]
        if pd == pre:
            return 'before'
        if pd == newd:
            return 'after'
        # sector-granular mix: every byte is old or new
        for i in range(3000):
            if pd[i] != pre[i] and pd[i] != newd[i]:
                return 'broken'
        return 'partial'
    if op == 'shrink':
        if pd is None:
            return 'broken'
        if pd == pre:
            return 'before'
        if pd == pre[:700]:
            return 'after'
        return 'broken'
    if op == 'trunc0':
        if pd is None:
            return 'broken'
        if pd == pre:
            return 'before'
        if pd == b'':
            return 'after'
        return 'broken'
    if op == 'remove':
        if pd == pre and 'PRE.DAT' in files:
            return 'before'
        if 'PRE.DAT' not in files:
            return 'after'
        return 'broken'
    if op == 'rename':
        a = 'PRE.DAT' in files
        b = 'MOVED.DAT' in files
        if a and not b and pd == pre:
            return 'before'
        if b and not a and moved[0] == pre:
            return 'after'
        return 'broken'
    return 'broken'


def fsck_rc(img):
    return subprocess.run(['fsck.fat', '-n', img], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode


def main():
    op, n, done, img, csv, prefix = sys.argv[1:7]
    v, errors, warns, files = files_of(img)
    sem = semantic(op, files)
    rc = fsck_rc(img)
    # repair a copy
    tmp = tempfile.mkdtemp()
    cp = os.path.join(tmp, 'r.img')
    shutil.copy(img, cp)
    vr = fc.Vol(cp, writable=True)
    log = fc.repair(vr)
    _, e2, w2, f2 = files_of(cp)
    rc2 = fsck_rc(cp)
    sem2 = semantic(op, f2)
    clean = (not e2 and not w2 and rc2 == 0)
    with open(csv, 'a') as f:
        f.write('%s,%s,%s,%d,%d,%d,%s,%s,%d\n' % (op, n, done, len(errors), len(warns), rc, sem, 'yes' if clean else 'NO', rc2))
    with open(prefix + '.judge.txt', 'w') as f:
        f.write('errors: %s\nwarnings: %s\nsemantic: %s\nrepair: %s\nafter repair: errors=%s warnings=%s fsck=%d semantic=%s\n'
                % (errors, warns, sem, log, e2, w2, rc2, sem2))
    shutil.rmtree(tmp)
    if sem2 == 'broken':
        print('BROKEN after repair: %s N=%s' % (op, n))
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
