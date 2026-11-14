#!/usr/bin/env python3
"""Assemble each Intel source and its AT&T conversion with the same modern assembler,
then compare .text instruction bytes and relocations, instruction by instruction."""
import glob, os, re, subprocess, sys
AS = 'i686-w64-mingw32-as'
SRC, ATT, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(OUT + '/intel', exist_ok=True); os.makedirs(OUT + '/att', exist_ok=True)

def asm(src_dir, name, out):
    r = subprocess.run([AS, '--defsym', 'MEMSIZE=65536', '-I', src_dir, '-o', out,
                        os.path.join(src_dir, name)], capture_output=True, text=True)
    return r.returncode, r.stderr

def text_items(obj):
    r = subprocess.run(['objdump', '-dr', '-j', '.text', obj], capture_output=True, text=True)
    items = []   # list of (addr, bytes, reloc-or-'', mnemonic)
    cur = None
    for line in r.stdout.split('\n'):
        m = re.match(r'^\s*([0-9a-f]+):\t((?:[0-9a-f]{2} ?)+)\s*\t?(.*)$', line)
        if m:
            cur = [int(m.group(1), 16), m.group(2).replace(' ', ''), '', m.group(3).strip()]
            items.append(cur)
            continue
        m = re.match(r'^\s*([0-9a-f]+): (R_\S+)\s+(\S+)', line)
        if m and cur is not None and int(m.group(1), 16) >= cur[0]:
            # relocation attaches to the instruction that contains that address
            for it in reversed(items):
                if it[0] <= int(m.group(1), 16):
                    it[2] += '%s:%s;' % (int(m.group(1), 16) - it[0], m.group(3)) if False else ''
                    it[2] += '%s@%d;' % (m.group(3), int(m.group(1), 16) - it[0])
                    break
    return items

names = sorted(os.path.basename(f) for f in glob.glob(SRC + '/*.S'))
bad = 0
for n in names:
    rc1, e1 = asm(SRC, n, OUT + '/intel/' + n[:-2] + '.o')
    rc2, e2 = asm(ATT, n, OUT + '/att/' + n[:-2] + '.o')
    if rc1 != 0 or rc2 != 0:
        print('%-18s ASM FAIL intel=%d att=%d %s' % (n, rc1, rc2, (e2 or e1)[:300].replace('\n', ' | ')))
        bad += 1
        continue
    a = text_items(OUT + '/intel/' + n[:-2] + '.o')
    b = text_items(OUT + '/att/' + n[:-2] + '.o')
    ka = [(x[1], x[2]) for x in a]
    kb = [(x[1], x[2]) for x in b]
    da = subprocess.run(['objdump', '-s', '-j', '.data', OUT + '/intel/' + n[:-2] + '.o'], capture_output=True, text=True).stdout
    db = subprocess.run(['objdump', '-s', '-j', '.data', OUT + '/att/' + n[:-2] + '.o'], capture_output=True, text=True).stdout
    dd = [l for l in da.split('\n') if re.match(r'^\s*[0-9a-f]+ ', l)] == [l for l in db.split('\n') if re.match(r'^\s*[0-9a-f]+ ', l)]
    if not dd:
        print('%-18s DATA DIFF' % n)
    if ka == kb and dd:
        print('%-18s OK   %5d insns, %d bytes' % (n, len(a), sum(len(x[1]) // 2 for x in a)))
    else:
        bad += 1
        i = next((k for k in range(min(len(ka), len(kb))) if ka[k] != kb[k]), min(len(ka), len(kb)))
        print('%-18s DIFF at insn %d (intel %d insns, att %d insns)' % (n, i, len(a), len(b)))
        for k in range(max(0, i - 2), min(i + 3, max(len(a), len(b)))):
            ia = a[k] if k < len(a) else None
            ib = b[k] if k < len(b) else None
            print('     intel: %s   att: %s' % (
                ('%04x %s %s  %s' % (ia[0], ia[1], ia[2], ia[3])) if ia else '-',
                ('%04x %s %s  %s' % (ib[0], ib[1], ib[2], ib[3])) if ib else '-'))
print('files with differences or errors: %d of %d' % (bad, len(names)))
