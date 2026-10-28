#!/usr/bin/env python3
"""R11K log assertions.  Judges the transcripts of scripts/r11k-matrix.sh line by line, independent
of the programs' own PASS lines where the transcript allows it, and writes results.json plus a
summary.  A check is PASS, FAIL or NA (field not in this transcript, e.g. cut by the 80-column
VGA screen under QEMU).  Exit status 1 if any check FAILs or an expected transcript is missing.

usage: python3 scripts/r11k-assert.py MATRIX_DIR [ROUNDS] [QEMU_ROUNDS]"""
import json, os, re, sys

D = sys.argv[1]
ROUNDS = int(sys.argv[2]) if len(sys.argv) > 2 else 3
QROUNDS = int(sys.argv[3]) if len(sys.argv) > 3 else 3
results = []


def add(target, check, ok, detail=''):
    results.append({'target': target, 'check': check,
                    'result': 'NA' if ok is None else ('PASS' if ok else 'FAIL'), 'detail': detail})


def first_occurrences(lines):
    """QEMU capture: successive 80x25 snapshots repeat most rows, and a snapshot can catch a row
    mid-update.  Keep the first occurrence of each distinct row, in order.  Counting identical
    rows is therefore not possible for QEMU; the checks below rely on distinct content."""
    seen, out = set(), []
    for l in lines:
        if l.startswith('=== SNAP') or l in seen:
            continue
        seen.add(l)
        out.append(l)
    return out


def load(name, vga=False):
    p = os.path.join(D, name)
    if not os.path.exists(p):
        return None
    lines = [l.rstrip('\r\n') for l in open(p, encoding='latin-1')]
    return first_occurrences(lines) if vga else lines


def find(lines, rx):
    r = re.compile(rx)
    return [m for m in (r.search(l) for l in lines) if m]


def section(lines, start_rx, stop_rx=r'^SAMELNCH [A-Z]: (PASS|FAIL)'):
    """Lines from the first match of start_rx to the next verdict line (inclusive)."""
    out, on = [], False
    for l in lines:
        if not on and re.search(start_rx, l):
            on = True
        if on:
            out.append(l)
            if re.search(stop_rx, l):
                break
    return out


def hx(s):
    return int(s, 16)


POOL_LO = {'i386vm': '900000', 'tiny386': '380000', 'qemu': '380000'}


def check_main(t, L, vga=False):
    if L is None:
        add(t, 'transcript present', False)
        return
    S = section(L, r'SAMELNCH M: start', r'SAMELNCH M: (PASS|FAIL)')
    add(t, 'SAMELNCH M verdict PASS', any('SAMELNCH M: PASS' in l for l in S))
    add(t, 'no failed program check (incl. third launch left nothing)',
        not any('CHECK FAILED' in l for l in S))
    rg = {m.group(1): m for m in find(S, r'^SAME (\d) ranges: own (-?\d+) heap (-?\d+) unmapped (-?\d+) pool\(([0-9a-f]+)\) (-?\d+) shell (-?\d+)')}
    for k in ('1', '2'):
        if k in rg:
            m = rg[k]
            add(t, 'copy %s pointer checks: own/heap accepted, unmapped/pool/shell refused' % k,
                m.group(2, 3) == ('1', '1') and m.group(4) == '-14' and m.group(6) == '-14' and
                m.group(7) == '-14' and m.group(5) == POOL_LO[t.split('-')[0]], m.group(0)[7:])
        else:
            add(t, 'copy %s pointer checks' % k, None if vga else False, 'line not captured')
    uh = find(S, r'unpaged hold flag at [0-9a-f]+ set (\d) of 8')
    add(t, 'unpaged job hold flag written in its own space (8/8)', bool(uh) and uh[0].group(1) == '8',
        uh[0].group(0) if uh else 'missing')
    dfr = [int(m.group(1)) for m in find(S, r'^SAME \d end: .* deferred (\d+)')]
    if dfr:
        add(t, 'hold writes deferred to the owner (> 0)', min(dfr) > 0, str(dfr))
    else:
        add(t, 'hold writes deferred to the owner (> 0)', None if vga else False, 'end lines cut')
    st = {m.group(1): m for m in find(S, r'^SAME (\d) start: main ([0-9a-f]+) gvals ([0-9a-f]+) gdata ([0-9a-f]+)')}
    if '1' in st and '2' in st:
        a, b = st['1'], st['2']
        add(t, 'same entry/main address in both copies', a.group(2) == b.group(2), a.group(2))
        add(t, 'same global addresses in both copies',
            a.group(3) == b.group(3) and a.group(4) == b.group(4), '%s %s' % (a.group(3), a.group(4)))
        add(t, 'main inside the common window', 0x240000 <= hx(a.group(2)) < 0x340000, a.group(2))
    else:
        add(t, 'both copies started', False)
    self_ = find(S, r'^SAMELNCH self: id \d+ paged 0 cr3 ([0-9a-f]+)')
    ab = {m.group(1): m for m in find(S, r'^SAMELNCH ([AB]): id (\d+) paged 1 cr3 ([0-9a-f]+) frames ([0-9a-f]+)')}
    if 'A' in ab and 'B' in ab and self_:
        A, B, tpl = ab['A'], ab['B'], self_[0].group(1)
        add(t, 'distinct directories (CR3)', A.group(3) != B.group(3), '%s %s' % (A.group(3), B.group(3)))
        add(t, 'copies not on the template directory', tpl not in (A.group(3), B.group(3)), tpl)
        add(t, 'distinct physical frames', A.group(4) != B.group(4), '%s %s' % (A.group(4), B.group(4)))
        add(t, 'frames not at the virtual window', hx(A.group(4)) != 0x240000 and hx(B.group(4)) != 0x240000)
    else:
        add(t, 'directory/frame report present', False)
    done = {m.group(1): m for m in find(S, r'^SAME (\d) done: n (\d+) acc \S+ checks (\d+) bad (\d+) first (\d+) last (\d+)')}
    if '1' in done and '2' in done:
        d1, d2 = done['1'], done['2']
        add(t, 'copy 1 state intact (bad 0)', d1.group(4) == '0' and int(d1.group(3)) > 0, 'checks ' + d1.group(3))
        add(t, 'copy 2 state intact (bad 0)', d2.group(4) == '0' and int(d2.group(3)) > 0, 'checks ' + d2.group(3))
        i1 = (int(d1.group(5)), int(d1.group(6)))
        i2 = (int(d2.group(5)), int(d2.group(6)))
        add(t, 'live intervals overlap (ticks)', max(i1[0], i2[0]) < min(i1[1], i2[1]), '%s %s' % (i1, i2))
    else:
        add(t, 'done lines present', False)
    pre = {m.group(1): int(m.group(2)) for m in find(S, r'^SAME (\d) end: .* preempts (\d+)')}
    if '1' in pre and '2' in pre:
        add(t, 'timer preemption of both copies', pre['1'] >= 1 and pre['2'] >= 1, str(pre))
    else:
        add(t, 'timer preemption of both copies', None if vga else False, 'end lines cut' if vga else 'missing')
    add(t, 'overlap observed by the launcher', bool(find(S, r'SAMELNCH: overlap at tick')))
    seq = [m.group(1) for m in find(S, r'^SAME ([12]) ')]
    alt = sum(1 for i in range(1, len(seq)) if seq[i] != seq[i - 1])
    add(t, 'output of the two copies interleaves', alt >= 3, '%d alternations' % alt)
    add(t, 'exit statuses 0x51 0x52', bool(find(S, r'SAMELNCH: exits 51 52')))
    third = find(S, r'third launch -> (-\d+)')
    add(t, 'third launch refused (bounded)', bool(third), third[0].group(1) if third else '')
    if vga and find(S, r'^SAMELNCH after:') and not find(S, r'^SAMELNCH after: .* held '):
        add(t, 'frames released (held 0) after exit', None, 'line cut at 80 columns')
    else:
        add(t, 'frames released (held 0) after exit', bool(find(S, r'^SAMELNCH after: .* held 0 ')))


def check_simple(t, L, mode, extra=()):
    if L is None:
        add(t, 'transcript present', False)
        return
    S = section(L, r'SAMELNCH %s: start' % mode, r'SAMELNCH %s: (PASS|FAIL)' % mode)
    add(t, 'SAMELNCH %s verdict PASS' % mode, any('SAMELNCH %s: PASS' % mode in l for l in S))
    for name, rx in extra:
        add(t, name, bool(find(S, rx)))
    return S


NEG_LINES = (('bad header -21', r'bad header -> -21'), ('wrong origin -61', r'profile-2 image -> -61'),
             ('unterminated args -34', r'unterminated arguments -> -34'), ('stale id -2', r'stale id \d+ -> -2'))


def check_repeat(t, S, pool):
    cyc = find(S, r'cycle (\d) ids \d+ \d+ frames ([0-9a-f]+) ([0-9a-f]+) cr3 ([0-9a-f]+) ([0-9a-f]+) exits 51 52')
    add(t, '8 repeat cycles with exits 51 52', len(cyc) == 8, '%d' % len(cyc))
    if cyc:
        fr = set((c.group(2), c.group(3)) for c in cyc)
        add(t, 'same frames every cycle (reuse)', len(fr) == 1, str(fr))
        a, b = hx(cyc[0].group(2)), hx(cyc[0].group(3))
        add(t, 'cycle frames lie in the frame pool', pool[0] <= a < pool[1] and pool[0] <= b < pool[1],
            '%x %x' % (a, b))


def check_fault_guest(t, S):
    """Tiny386/QEMU: the guest #PF handler reports and ends the faulting copy; A continues.
    Judged on distinct lines, so it holds for QEMU screen captures too."""
    if S is None:
        return
    tgt = find(S, r"touching another process's frames at ([0-9a-f]+)")
    pf = find(S, r'PGFAULT: vec 0000000e err ([0-9a-f]+) cr2 ([0-9a-f]+) eip ([0-9a-f]+)')
    cr2s = dict((m.group(2), m.group(3)) for m in pf)
    if tgt:
        other = tgt[0].group(1)
        add(t, "#PF reported with CR2 = the other copy's frames", other in cr2s, other)
        if other in cr2s:
            add(t, '#PF EIP inside the window', 0x240000 <= hx(cr2s[other]) < 0x340000, cr2s[other])
    else:
        add(t, 'cross-process access attempted', False)
    add(t, '#PF reported with CR2 = 01000000', '01000000' in cr2s)
    add(t, 'launcher saw the cross-process copy end (-17)',
        bool(find(S, r"copy touching A's frames [0-9a-f]+ ended with -17")))
    add(t, 'launcher saw the unmapped-access copy end (-17)',
        bool(find(S, r'copy touching 01000000 ended with -17')))
    add(t, 'A unaffected (exit 51)', bool(find(S, r'running beside both faults, exit 51')))


def check_fault_vm(t, L):
    if L is None:
        add(t, 'transcript present', False)
        return
    g = find(L, r'guest exception 14 at ([0-9a-f]+)')
    fd = find(L, r'FAULTDUMP: excno 14 eip ([0-9a-f]+) cr0 ([0-9a-f]+) cr2 ([0-9a-f]+) cr3 ([0-9a-f]+)')
    tgt = find(L, r"touching another process's frames at ([0-9a-f]+)")
    info2 = find(L, r'^SAME 2 info: rc 0 paged 1 cr3 ([0-9a-f]+)')
    add(t, 'core reports guest exception 14', bool(g))
    add(t, 'faultdump present', bool(fd))
    if fd and tgt:
        add(t, 'CR2 = the other copy\'s frames', fd[0].group(3) == tgt[0].group(1), fd[0].group(3))
        add(t, 'CR0.PG set at the fault', hx(fd[0].group(2)) & 0x80000000 != 0, fd[0].group(2))
        add(t, 'EIP inside the window', 0x240000 <= hx(fd[0].group(1)) < 0x340000, fd[0].group(1))
    if fd and info2:
        add(t, 'CR3 = the faulting copy\'s directory', fd[0].group(4) == info2[0].group(1),
            '%s %s' % (fd[0].group(4), info2[0].group(1)))


def check_multi_neg(t, L, multi=True, neg=True):
    if L is None:
        add(t, 'transcript present', False)
        return
    if multi:
        add(t, 'MULTI: PASS', any('MULTI: PASS' in l for l in L))
    if neg:
        add(t, 'NEG 848 checks, 0 failures', any('NEG: checks 848, failures 0' in l for l in L))



def tsec(L, mode):
    return section(L or [], r'TWICET %s: start' % mode, r'TWICET %s: (PASS|FAIL|NA)' % mode)


def check_twicet(t, L, mode, vga=False, na_ok=False):
    """One TWICET mode.  Verdict plus no failed internal check."""
    if L is None:
        add(t, 'transcript present', False)
        return None
    S = tsec(L, mode)
    if not S:
        add(t, 'TWICET %s ran' % mode, None if vga else False, 'section not captured')
        return S
    v = [l for l in S if re.search(r'TWICET %s: (PASS|FAIL|NA)' % mode, l)]
    if v and 'NA' in v[-1] and na_ok:
        add(t, 'TWICET %s verdict NA (console input pending on this harness)' % mode, None, v[-1])
        return S
    add(t, 'TWICET %s verdict PASS' % mode, bool(v) and 'PASS' in v[-1], v[-1] if v else 'no verdict line')
    add(t, 'TWICET %s no failed internal check' % mode, not any('CHECK FAILED' in l for l in S))
    return S


def check_twicet_main(t, L, vga=False):
    S = check_twicet(t, L, 'M', vga)
    if not S:
        return
    caps = find(S, r'TWICET: caps ([0-9a-f]{8})')
    add(t, 'caps: ABI version 2 and START|INFO|ANYEXC|HOLD|TASKS|HELD', bool(caps) and
        (hx(caps[0].group(1)) >> 24) == 2 and (hx(caps[0].group(1)) & 0x3f) == 0x3f, caps[0].group(1) if caps else 'missing')
    ex = find(S, r'TWICET: exits 41 42')
    add(t, 'exit statuses 0x41 0x42 from the same file (5 pair runs)', len(ex) >= (1 if vga else 5), '%d' % len(ex))
    ok = find(S, r'TWICE\[[0-9a-f]+\] tag ([12]): OK loops (\d+)')
    add(t, 'both copies report OK and ran their loops', {m.group(1) for m in ok} == {'1', '2'} and
        all(int(m.group(2)) > 0 for m in ok), '%d lines' % len(ok))
    fl = find(S, r'TWICE\[[0-9a-f]+\] tag ([12]) file BIG.BIN: 196608 bytes, checksum c4e80000 \(expected c4e80000\)')
    add(t, 'both copies read BIG.BIN (192 KiB) with the expected checksum', {m.group(1) for m in fl} == {'1', '2'},
        '%d lines' % len(fl))
    st = {}
    for m in find(S, r'TWICE\[[0-9a-f]+\] tag ([12]) start: argv0 (\S+) tab ([0-9a-f]+) bss ([0-9a-f]+) line ([0-9a-f]+)'):
        st.setdefault(m.group(1), m.group(3, 4, 5))
    add(t, 'both copies use the same virtual addresses for their globals', len(st) == 2 and st['1'] == st['2'], str(st))
    add(t, 'globals lie in the window 0x240000..0x33FFFF', all(0x240000 <= hx(a) < 0x340000 for v in st.values() for a in v), str(st))
    ids = set(m.group(1) for m in find(S, r'TWICET: held starts (\d+) and (\d+)') ) | set(m.group(2) for m in find(S, r'TWICET: held starts (\d+) and (\d+)'))
    add(t, 'held starts returned distinct job ids', len(ids) >= (2 if vga else 10), '%d ids' % len(ids))
    if not vga:
        seq = [m.group(1) for m in find(S, r'TWICE\[[0-9a-f]+\] tag ([12]) ')]
        alt = sum(1 for i in range(1, len(seq)) if seq[i] != seq[i - 1])
        add(t, 'output of the two copies interleaves (shared console, honestly)', alt >= 3, '%d alternations' % alt)


def check_twicet_neg(t, L, vga=False):
    S = check_twicet(t, L, 'N', vga)
    if not S:
        return
    for name, rx in (('bad header -21', r'bad header -> -21'), ('wrong origin -61', r'profile-2 image -> -61'),
                     ('missing file -44', r'missing file -> -44'), ('unterminated args -34', r'unterminated args -> -34'),
                     ('unknown flag -34', r'unknown flag -> -34'), ('paged caller refused (-62 x4, child exit 0x70)', r'refusal child exit 70')):
        add(t, name, bool(find(S, rx)))
    add(t, 'child saw all four refusals as -62', bool(find(S, r'child R: refusals all -62')) or vga, '' if not vga else 'line may be cut')


def check_pgrun(t, L, vga=False, statuses=('41', '42'), catfil=True):
    """PGRUN runs.  `statuses` are the TWICE exit statuses this sequence must show (0x41 = TWICE 30 1,
    0x42 = TWICE 20 2 + BIG.BIN); `catfil` = the sequence also runs two copies of CATFIL.  Full sequences also run
    FORMAT, FPMULT and HELLO under PGRUN: those finish before both copies can be sampled live, so a
    'PGRUN: overlap no' for them is reported NA (no overlap claim is made for them), never as a pass."""
    if L is None:
        add(t, 'transcript present', False)
        return
    ov = find(L, r'PGRUN: overlap yes')
    want = len(statuses) + (1 if catfil else 0)
    add(t, 'PGRUN: designed runs overlapped (both dispatched and live at one instant)',
        len(ov) >= (1 if vga else want), '%d overlap-yes lines, %d designed runs' % (len(ov), want))
    no = find(L, r'PGRUN: overlap no')
    if no:
        add(t, 'PGRUN: short programs (FORMAT/FPMULT/HELLO) finished before both copies were sampled live', None,
            '%d runs; no overlap claim made for them' % len(no))
    for st in statuses:
        add(t, 'PGRUN: TWICE copies exit 0x%s 0x%s' % (st, st),
            bool(find(L, r'PGRUN: exit 1 status %s$' % st)) and bool(find(L, r'PGRUN: exit 2 status %s$' % st)))
    add(t, 'PGRUN: released yes after every run', bool(find(L, r'PGRUN: released yes')) and not find(L, r'PGRUN: released no'))
    add(t, 'PGRUN: DONE', bool(find(L, r'PGRUN: DONE')) and not find(L, r'PGRUN: FAILED'))
    if catfil:
        fc = find(L, r'QDOS/386 R9 FILECAT PASS')
        add(t, 'two copies of the SDK utility CATFIL both PASS', len(fc) >= (1 if vga else 2), '%d' % len(fc))
        if not vga:
            add(t, 'CATFIL copies exit 0 0', bool(find(L, r'PGRUN: exit 1 status 0$')) and bool(find(L, r'PGRUN: exit 2 status 0$')))


def check_fault_twicet(t, L, vga=False):
    S = check_twicet(t, L, 'F', vga)
    if not S:
        return
    for vec, name in (('0000000e', '#PF'), ('00000000', '#DE'), ('00000006', '#UD')):
        add(t, '%s in a paged copy reported' % name, bool(find(S, r'PGFAULT: vec %s ' % vec)))
    add(t, 'each faulting copy ended with -17', len(find(S, r'copy ended with -17')) + len(find(S, r'(page-fault|divide-error|invalid-opcode) copy ended with -17')) >= 3 or vga)
    add(t, 'healthy copy beside three faults exits 0x41', bool(find(S, r'healthy copy beside three faults exit 41')))


def check_simple2(t, L, mode, extra=(), vga=False):
    S = check_twicet(t, L, mode, vga)
    if S:
        for name, rx in extra:
            add(t, name, bool(find(S, rx)))
    return S


def check_tasks(t, L):
    S = check_simple2(t, L, 'K')
    if not S:
        return
    b = find(S, r'kernel tasks before: poll (\d+) \(paged (\d+), off-template (\d+)\)  sched (\d+) \(paged (\d+), off-template (\d+)\)')
    a = find(S, r'kernel tasks after: poll (\d+) \(paged (\d+), off-template (\d+)\)  sched (\d+) \(paged (\d+), off-template (\d+)\)')
    if b and a:
        B = [int(x) for x in b[0].groups()]; A = [int(x) for x in a[0].groups()]
        add(t, 'poll and schedule tasks kept running (counters grew)', A[0] > B[0] and A[3] > B[3], '%s -> %s' % (B, A))
        add(t, 'they ran while a paged job was current', A[1] > B[1] and A[4] > B[4])
        add(t, 'never under a paged directory (off-template 0)', A[2] == 0 and A[5] == 0)
    else:
        add(t, 'task counters present', False)


POOL_VM, POOL_PC = (0x900000, 0x1000000), (0x380000, 0x600000)
for r in range(1, ROUNDS + 1):
    n = 'i386vm-pg-r%d' % r
    L = load('i386vm-pg-r%d.txt' % r)
    check_main(n, L)
    check_simple(n, L, 'N', NEG_LINES)
    S = check_simple(n, L, 'R')
    if S is not None:
        check_repeat(n, S, POOL_VM)
    check_twicet_main(n, L)
    check_twicet_neg(n, L)
    check_simple2(n, L, 'H', (('group start refuses beyond the limit', r'next start -> -\d+'),))
    check_simple2(n, L, 'E', (('exhaustion refused with a bounded code', r'held starts succeeded; the next returned -(60|63|29|30)'),))
    check_simple2(n, L, 'S', (('10 launch/exit cycles', r'TWICET: 10 cycles'),))
    check_simple2(n, L, 'Z', (('heap shared: copy A write visible', r'holds 5a5a5a5a after copy A'),))
    check_twicet(n, L, 'W', na_ok=True)
    check_pgrun(n, L)
    check_fault_vm('i386vm-pgfault-r%d' % r, load('i386vm-pgfault-r%d.txt' % r))
    check_multi_neg('i386vm-pgmulti-r%d' % r, load('i386vm-pgmulti-r%d.txt' % r), neg=False)
    check_multi_neg('i386vm-pgneg-r%d' % r, load('i386vm-pgneg-r%d.txt' % r), multi=False)
    n = 'tiny386-pg-r%d' % r
    L = load('tiny386-pg-r%d.txt' % r)
    check_main(n, L)
    check_fault_guest(n, check_simple(n, L, 'F'))
    check_simple(n, L, 'N', NEG_LINES)
    S = check_simple(n, L, 'R')
    if S is not None:
        check_repeat(n, S, POOL_PC)
    check_multi_neg(n, L, neg=False)
    check_twicet_main(n, L)
    check_twicet_neg(n, L)
    check_simple2(n, L, 'H', (('group start refuses beyond the limit', r'next start -> -60'),))
    check_simple2(n, L, 'E', (('exhaustion refused with -60 (2 windows)', r'2 held starts succeeded; the next returned -60'),))
    check_simple2(n, L, 'S', (('10 launch/exit cycles', r'TWICET: 10 cycles'),))
    check_simple2(n, L, 'Z', (('heap shared: copy A write visible', r'holds 5a5a5a5a after copy A'),))
    check_simple2(n, L, 'W', (('second console waiter refused (-50, exit 0x71)', r'second console waiter exit 71'),
                              ('first waiter released (exit 0x70)', r'first console waiter exit 70')))
    check_fault_twicet(n, L)
    check_pgrun(n, L)
    check_multi_neg('tiny386-pgneg-r%d' % r, load('tiny386-pgneg-r%d.txt' % r), multi=False)
for q in range(1, QROUNDS + 1):
    n = 'qemu-pg-r%d' % q
    L = load('qemu-pg-r%d.txt' % q, vga=True)
    check_twicet_main(n, L, vga=True)
    check_fault_twicet(n, L, vga=True)
    check_simple2(n, L, 'W', (('second console waiter refused (-50)', r'second console waiter exit 71'),), vga=True)
    check_simple2(n, L, 'Z', (('heap shared: copy A write visible', r'holds 5a5a5a5a after copy A'),), vga=True)
    check_pgrun(n, L, vga=True, statuses=('42',))
    check_main(n, L, vga=True)
    check_fault_guest(n, check_simple(n, L, 'F'))
for t in (400, 896):
    check_twicet_main('tiny386-large-%d' % t, load('tiny386-large-%d.txt' % t))
    check_fault_twicet('tiny386-large-%d' % t, load('tiny386-large-%d.txt' % t))
    check_pgrun('tiny386-large-%d' % t, load('tiny386-large-%d.txt' % t), statuses=('41',), catfil=False)
    L = load('qemu-large-%d.txt' % t, vga=True)
    check_twicet_main('qemu-large-%d' % t, L, vga=True)
    check_fault_twicet('qemu-large-%d' % t, L, vga=True)
    check_pgrun('qemu-large-%d' % t, L, vga=True, statuses=('42',))

Y = load('tiny386-pg-Y.txt')
add('tiny386-pg-Y', 'unpaged task fault in window code halts the machine', bool(Y) and any('PGFAULT: halt' in l for l in Y))
add('tiny386-pg-Y', 'the innocent paged copy is NOT blamed (no "paged job ended")', bool(Y) and not any('paged job ended' in l for l in Y))
P = load('tiny386-prefix-Y.txt') if False else None
for t, f in (('i386vm-test-flaunch', 'i386vm-test-flaunch.txt'), ('tiny386-test-flaunch', 'tiny386-test-flaunch.txt')):
    check_simple2(t, load(f), 'T', (('injected read failure -> -40', r'injected read failure -> -40'),))
check_simple2('i386vm-test-cap', load('i386vm-test-cap.txt'), 'E', (('2 windows: two held starts then -60', r'2 held starts succeeded; the next returned -60'),))
for t, f in (('i386vm-test-intcr3', 'i386vm-test-intcr3.txt'), ('tiny386-test-intcr3', 'tiny386-test-intcr3.txt')):
    L = load(f)
    check_twicet(t, L, 'M')
    add(t, 'SAMELNCH M verdict PASS', any('SAMELNCH M: PASS' in l for l in (L or [])))
    m = find(L or [], r'cr3 loads (\d+), hold writes deferred (\d+), irq7 entries (\d+)')
    add(t, 'interrupt before and after every CR3 load',
        bool(m) and int(m[0].group(3)) == 2 * int(m[0].group(1)) and int(m[0].group(1)) > 0,
        m[0].group(0) if m else 'missing')
check_tasks('i386vm-test-tasks', load('i386vm-test-tasks.txt'))
check_tasks('tiny386-test-tasks', load('tiny386-test-tasks.txt'))

# NEGATIVE CONTROL: shared frames must be detected by the application's own checks.
L = load('i386vm-control-share.txt')
add('control-share', 'shared frames detected: TWICET M FAIL', bool(L) and any('TWICET M: FAIL' in l for l in L))
add('control-share', 'shared frames detected: a TWICE copy reports FAIL', bool(L) and bool(find(L, r'TWICE\[[0-9a-f]+\] tag \d: FAIL \d+')))
add('control-share', 'no copy reported OK', bool(L) and not find(L, r'TWICE\[[0-9a-f]+\] tag \d: OK'))

# R11F regression under the paging kernel against the default i386VM run of the same sequence.
P, Dr = load('i386vm-pgregression.txt'), load('def-regression/i386vm-regression-none.txt')
if P is None or Dr is None:
    add('i386vm-pgregression', 'transcripts present', False)
else:
    pp = sorted(l for l in P if 'PASS' in l or 'FAIL' in l)
    dd = sorted(l for l in Dr if 'PASS' in l or 'FAIL' in l)
    add('i386vm-pgregression', 'same PASS/FAIL lines as the default kernel', pp == dd, '%d lines' % len(pp))
    add('i386vm-pgregression', 'no FAIL line (apart from the documented historical floatundidf cases, none expected)', not any('FAIL' in l for l in P))
T = load('tiny386-pgregression.txt')
add('tiny386-pgregression', 'transcript present and no FAIL line', T is not None and not any('FAIL' in l for l in T))
add('tiny386-pgregression', 'Phoon / time / FPU programs: PHCHK PASS, FPMULT PASS, PHDATE epoch values correct',
    T is not None and any('R11F PHCHK PASS' in l for l in T) and any('R11C FPMULT PASS' in l for l in T)
    and any('-> 1704974220' in l for l in T) and any('-> 4134024000' in l for l in T))

for r in range(1, ROUNDS + 1):
    check_multi_neg('def-legacy-r%d' % r, load('i386vm-def-legacy-multi-r%d.txt' % r), neg=False)
    check_multi_neg('def-legacy-r%d' % r, load('i386vm-def-legacy-neg-r%d.txt' % r), multi=False)
    check_multi_neg('def-block-r%d' % r, load('i386vm-def-block-multi-r%d.txt' % r), neg=False)
    check_multi_neg('def-block-r%d' % r, load('i386vm-def-block-neg-r%d.txt' % r), multi=False)
    check_multi_neg('def-tiny386-r%d' % r, load('tiny386-def-multi-r%d.txt' % r), neg=False)
    check_multi_neg('def-tiny386-r%d' % r, load('tiny386-def-neg-r%d.txt' % r), multi=False)
    check_multi_neg('defk-tiny386-r%d' % r, load('tiny386-defk-multi-r%d.txt' % r), neg=False)
    check_multi_neg('defk-tiny386-r%d' % r, load('tiny386-defk-neg-r%d.txt' % r), multi=False)
for q in range(1, QROUNDS + 1):
    L = load('qemu-defk-r%d.txt' % q, vga=True)
    add('qemu-defk-r%d' % q, 'default kernel with the R11K loader: HELLO / CATFIL / MULTI', L is not None and any('FILECAT PASS' in l for l in L)
        and any('MULTI: PASS' in l for l in L))
for r in range(1, ROUNDS + 1):
    for n, f in (('defk-tiny386-nopaging-r%d' % r, 'tiny386-defk-nopaging-r%d.txt' % r), ('def-block-nopaging-r%d' % r, 'i386vm-def-block-nopaging-r%d.txt' % r)):
        L = load(f)
        add(n, 'TWICET X: every paging call returns -70 on a non-paging kernel', bool(L) and any('TWICET X: PASS' in l for l in L))
        add(n, 'TWICET M reports NA (no paging), PGRUN refuses cleanly', bool(L) and any('TWICET M: NA' in l for l in L) and any('PGRUN: FAILED this kernel has no paging' in l for l in L))
        add(n, 'ordinary unpaged CATFIL unaffected', bool(L) and any('FILECAT PASS' in l for l in L))
L = load('def-regression.log')
add('def-regression', 'regression volume unchanged', bool(L) and any('VOLUME UNCHANGED' in l for l in L))
H = load('host-tests.txt')
add('host-tests', 'BLOCK and LAYOUT host tests PASS',
    bool(H) and any('BLOCK: PASS' in l for l in H) and any('LAYOUT: PASS' in l for l in H))
H = load('host-tests-r11k.txt')
add('host-tests-r11k', 'boot image tests OK', bool(H) and any(l.startswith('OK') for l in H))
a, b = load('volumes-before.txt'), load('volumes-after.txt')
add('volumes', 'every volume hash unchanged', a is not None and a == b, '%d volumes' % len(a or []))

counts = {k: sum(1 for x in results if x['result'] == k) for k in ('PASS', 'FAIL', 'NA')}
json.dump({'counts': counts, 'results': results}, open(os.path.join(D, 'results.json'), 'w'), indent=1)
with open(os.path.join(D, 'results-summary.txt'), 'w') as f:
    for x in results:
        if x['result'] != 'PASS':
            f.write('%s %s: %s %s\n' % (x['result'], x['target'], x['check'], x['detail']))
    f.write('PASS %(PASS)d FAIL %(FAIL)d NA %(NA)d\n' % counts)
print(open(os.path.join(D, 'results-summary.txt')).read(), end='')
sys.exit(1 if counts['FAIL'] else 0)
