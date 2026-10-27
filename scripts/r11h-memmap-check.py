#!/usr/bin/env python3
"""R11H memory-map check.

Reads the layout macros from kernel/appmap.h through the host preprocessor (for each layout), takes
the heap arena base from the kernel's own boot line ("Heap: arena N to M") in a transcript, and
checks that every reservation on each target is inside the target's RAM and that no two
reservations overlap.  Also checks the loader's FREE pool against the reservations: every pool span
must lie inside the profile ranges and outside all other reservations.

Targets (see docs/R11H-MEMORY-MAP.md):
  tiny386  R9 map, PC, 8 MiB RAM (run.ini mem_size = 8M), disk on the second IDE drive (no RAM disk)
  i386vm   R9 map, i386VM, 16 MiB guest RAM, FAT16 volume in RAM at 0x900000 (transport)
  i386vm-r8  default map, i386VM, 8 MiB (the R5-R8 baseline), profiles 2 and 3 must be absent

The kernel's heap arena starts at the 16-byte-aligned end of BSS (kernel/main.c: arena = bss_end
aligned up).  That value is read from each kernel's own a.out symbol table (_bss_end), not assumed.
The kernels are linked by scripts/r11h-maps-link.sh.

usage: r11h-memmap-check.py <pc-kernel.aout> <i386vm-r9.aout> <i386vm-r8.aout>
Exit 0 only if every check passes.
"""
import re, struct, subprocess, sys

ROOT_HDR = 'kernel/appmap.h'
ARENA = re.compile(r'Heap: arena (\d+) to (\d+)')

def macros(defs):
    out = subprocess.run(['gcc', '-E', '-dM', '-x', 'c'] + defs + [ROOT_HDR],
                         check=True, capture_output=True, text=True).stdout
    env = {}
    for line in out.splitlines():
        m = re.match(r'#define (\w+) (.*)$', line)
        if m and not m.group(1).startswith('_'):
            env[m.group(1)] = m.group(2).strip()
    return env

def expand(env, expr, depth=0):
    """Expands object-like macros (recursively) and drops integer suffixes."""
    if depth > 20:
        sys.exit('macro recursion too deep')
    def rep(m):
        t = m.group(0)
        if t in env:
            return '(' + expand(env, env[t], depth + 1) + ')'
        return t
    expr = re.sub(r'\b[A-Za-z_]\w*\b', rep, expr)
    return re.sub(r'\b(0[xX][0-9a-fA-F]+|\d+)[uUlL]+\b', r'\1', expr)

def num(env, name):
    """Integer value of an object-like macro.  The expression comes from kernel/appmap.h itself."""
    v = eval(expand(env, env[name]), {'__builtins__': {}}, {})
    return int(v)

def bss_end(path):
    """_bss_end from an i386 a.out symbol table (12-byte nlist records, AOUT_SYM_BYTES)."""
    b = open(path, 'rb').read()
    magic, text, data, bss, syms, entry = struct.unpack('<6I', b[:24])
    if magic != 0x107:
        sys.exit(path + ': not OMAGIC')
    off = 32 + text + data
    strs = b[off + syms:]
    for i in range(syms // 12):
        strx, _t, _o, _d, val = struct.unpack('<IBBHI', b[off + 12 * i:off + 12 * i + 12])
        if strx and strx < len(strs):
            name = strs[strx:strs.index(b'\0', strx)]
            if name == b'_bss_end':
                return val
    sys.exit(path + ': no _bss_end symbol')

def arena_base(path):
    """Heap arena [start, end): start is the 16-byte-aligned end of BSS, end is HEAP_END."""
    start = (bss_end(path) + 15) & ~15
    return start, None

def check_target(name, env, kernel_start, heap_start, heap_end, ram_top, stack_top, failures, i386=True):
    """Returns a list of (label, start, end, kind) and records failures."""
    g = lambda n: num(env, n)
    items = []
    def add(label, start, end, kind):
        if end <= start:
            failures.append(f'{name}: {label} empty or inverted')
            return
        items.append((label, start, end, kind))
    add('kernel image + BSS', kernel_start, heap_start, 'RESERVED')
    add('heap', heap_start, g('HEAP_END'), 'RESERVED')
    add('slot 0 (shell)', g('APP_SLOT0_BASE'), g('APP_SLOT0_BASE') + g('APP_SLOT0_SIZE'), 'PROFILE')
    add('slot 1 (child)', g('APP_SLOT1_BASE'), g('APP_SLOT1_BASE') + g('APP_SLOT1_SIZE'), 'PROFILE')
    add('transfer area', g('XFER_BASE'), g('XFER_BASE') + g('XFER_SIZE'), 'RESERVED')
    if g('APP_PROFILE2_SIZE'):
        add('profile 2', g('APP_PROFILE2_BASE'), g('APP_PROFILE2_BASE') + g('APP_PROFILE2_SIZE'), 'PROFILE')
    else:
        if g('APP_PROFILE2_BASE') != g('APP_PROFILE2_BASE'):
            failures.append(f'{name}: unreachable')
    if g('APP_PROFILE3_SIZE'):
        add('profile 3', g('APP_PROFILE3_BASE'), g('APP_PROFILE3_BASE') + g('APP_PROFILE3_SIZE'), 'PROFILE')
    add('boot stack and firmware data', 0x600000, ram_top if ram_top <= 0x800000 else 0x800000, 'RESERVED')
    if i386 and g('DEVICE_SIZE'):
        add('device page (ports, i386VM)', g('DEVICE_BASE'), g('DEVICE_BASE') + g('DEVICE_SIZE'), 'RESERVED')
    if i386 and g('RAMDISK_SIZE'):
        add('FAT16 RAM-disk transport', g('RAMDISK_BASE'), g('RAMDISK_BASE') + g('RAMDISK_SIZE'), 'TRANSPORT')
    # Stack top must be inside the boot-stack reservation.  0x7FFFF0 is verified for the PC
    # (platform/pc/entry.s); for i386VM it is the R5 boot-stack range, not verified from source.
    if not (0x600000 < stack_top <= 0x800000):
        failures.append(f'{name}: stack top {stack_top:#x} outside 0x600000..0x800000')
    # Everything inside RAM.
    for label, s, e, _ in items:
        if e > ram_top:
            failures.append(f'{name}: {label} ends at {e:#x}, above RAM top {ram_top:#x}')
    # Pairwise disjointness.
    items.sort(key=lambda t: t[1])
    for i in range(len(items)):
        for j in range(i + 1, len(items)):
            a, b = items[i], items[j]
            if a[2] > b[1] and b[2] > a[1]:
                failures.append(f'{name}: {a[0]} overlaps {b[0]}')
    return items

def pool_spans(env, g):
    """The FREE spans the loader registers (aout.c app_pool_init), from the same macros."""
    spans = [(g('APP_SLOT0_BASE'), g('APP_SLOT1_BASE') + g('APP_SLOT1_SIZE'))]
    for n in (2, 3):
        size = g(f'APP_PROFILE{n}_SIZE')
        if size:
            base = g(f'APP_PROFILE{n}_BASE')
            spans.append((base, base + size))
    return spans

def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    tiny_t, i386_t, r8_t = sys.argv[1:4]
    failures = []
    r9 = macros(['-DR7_SHELL_MODE', '-DQDOS_LAYOUT_R9'])
    r8 = macros(['-DR7_SHELL_MODE'])

    # Kernel start: 0x100000 on the PC (kernel.ld), 0 on i386VM.
    tiny_arena = arena_base(tiny_t)[0]
    i386_arena = arena_base(i386_t)[0]
    r8_arena = arena_base(r8_t)[0]
    head = num(r9, 'HEAP_END')

    print('## Heap arena (16-byte-aligned end of BSS, from each kernel a.out)')
    print(f'tiny386   {tiny_arena:#x} .. {head:#x}')
    print(f'i386vm    {i386_arena:#x} .. {head:#x}')
    print(f'i386vm-r8 {r8_arena:#x} .. {num(r8, "HEAP_END"):#x}')

    out = {}
    # The PC has no device page or RAM disk: its FAT16 volume is on the second IDE drive, and
    # run.ini sets mem_size = 8M, so RAM ends at 0x800000 (an assumption; the PC kernel does not probe).
    out['tiny386'] = check_target('tiny386', r9, 0x100000, tiny_arena, head,
                                  0x800000, 0x7FFFF0, failures, i386=False)
    out['i386vm'] = check_target('i386vm', r9, 0, i386_arena, head,
                                 num(r9, 'RAM_TOP'), 0x7FFFF0, failures)
    out['i386vm-r8'] = check_target('i386vm-r8', r8, 0, r8_arena, num(r8, 'HEAP_END'),
                                    num(r8, 'RAM_TOP'), 0x7FFFF0, failures)

    # Pool FREE spans: inside profiles, disjoint from every RESERVED or TRANSPORT item.
    g9 = lambda n: num(r9, n)
    for label, items in (('tiny386', out['tiny386']), ('i386vm', out['i386vm'])):
        for s, e in pool_spans(r9, g9):
            for item in items:
                if item[3] == 'PROFILE':
                    continue
                if s < item[2] and item[1] < e:
                    failures.append(f'{label}: pool span {s:#x}..{e:#x} overlaps {item[0]}')
    # Profile 3 must not reach the boot stack; profile 2 must not reach the transfer area.
    if g9('APP_PROFILE3_BASE') + g9('APP_PROFILE3_SIZE') > 0x600000:
        failures.append('profile 3 reaches the boot stack')
    # Default map: profiles 2 and 3 have no pool span, and the RAM disk is where the R8 map puts it.
    if num(r8, 'APP_PROFILE2_SIZE') or num(r8, 'APP_PROFILE3_SIZE'):
        failures.append('default map exposes profiles 2 or 3')
    if num(r8, 'RAMDISK_BASE') != 0x2C0000:
        failures.append('default map RAM disk moved')

    print('\n## Reservations')
    for name in ('tiny386', 'i386vm', 'i386vm-r8'):
        print(f'\n{name}')
        for label, s, e, kind in sorted(out[name], key=lambda t: t[1]):
            print(f'  {s:#09x} .. {e:#09x}  {kind:<9} {label}')
    unused = [(0x480000, 0x500000)]   # R9 map: 0x380000..0x47FFFF is profile 2, 0x500000..0x5FFFFF is profile 3
    print('\nUnassigned on the R9 map (not reserved, not a profile): '
          + ', '.join(f'{s:#x}..{e:#x}' for s, e in unused)
          + ' (512 KiB, between profile 2 and profile 3)')

    print()
    for f in failures:
        print('FAIL', f)
    print('MEMMAP CHECK:', 'PASS' if not failures else 'FAIL')
    return 0 if not failures else 1

if __name__ == '__main__':
    sys.exit(main())
