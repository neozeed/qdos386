#!/usr/bin/env python3
"""Convert the GW-BASIC port's Intel-syntax GNU as sources (.intel_syntax noprefix)
to AT&T syntax for GNU as 1.38, which has no Intel mode.

Usage: intel2att.py SRCDIR OUTDIR [--inline] [--memsize N]

--inline     replace each .include with the converted include file (GNU as 1.38 has no .include)
--memsize N  define MEMSIZE in every file that uses it (instead of assembler --defsym)

One output line per input line, so an assembler mismatch maps back to a source line.
Bare symbols are classified as GAS Intel does: an absolute value (.set / = of constants)
is an immediate; anything label-relative (a label, .lcomm, .extern, or a .set / =
built from one) is a memory operand. OFFSET always gives an immediate address.
Char literals become decimal numbers (GAS 1.38 reads '#' and '$' oddly in them).
Errors are collected and listed; the converter exits non-zero if any line failed.
"""
import glob
import os
import re
import sys

REG8 = {'al', 'ah', 'bl', 'bh', 'cl', 'ch', 'dl', 'dh'}
REG16 = {'ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp', 'sp'}
REG32 = {'eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp'}
REGS = REG8 | REG16 | REG32
SEGS = {'cs', 'ds', 'es', 'fs', 'gs', 'ss'}
KEYWORDS = REGS | SEGS | {'offset', 'ptr', 'byte', 'word', 'dword', 'short', 'near'}
PTRSZ = {'BYTE': 'b', 'WORD': 'w', 'DWORD': 'l'}

# Two-operand (and one-operand) ALU/move forms that take a size suffix.
SUFFIXED = {'mov', 'add', 'sub', 'cmp', 'and', 'or', 'xor', 'test', 'adc', 'sbb',
            'inc', 'dec', 'neg', 'not', 'mul', 'imul', 'div', 'idiv',
            'push', 'pop', 'xchg'}
SHIFTS = {'shl', 'shr', 'sar', 'sal', 'rol', 'ror', 'rcl', 'rcr'}
# Operand-less spellings that differ between Intel and AT&T.
RENAME0 = {'pushfd': 'pushfl', 'popfd': 'popfl', 'cbw': 'cbtw', 'cwde': 'cwtl',
           'cdq': 'cltd', 'cwd': 'cwtd'}
PREFIXES = {'rep', 'repe', 'repne', 'repz', 'repnz', 'lock'}
# String instructions with implicit operands. GAS 1.38 has no segment-override operand
# syntax for them, so they are emitted as their opcode bytes (checked against the Intel build).
STRING_OPS = {
    'movsb': ['0xa4'], 'movsw': ['0x66', '0xa5'], 'movsd': ['0xa5'],
    'cmpsb': ['0xa6'], 'cmpsw': ['0x66', '0xa7'], 'cmpsd': ['0xa7'],
    'lodsb': ['0xac'], 'lodsw': ['0x66', '0xad'], 'lodsd': ['0xad'],
    'stosb': ['0xaa'], 'stosw': ['0x66', '0xab'], 'stosd': ['0xab'],
    'scasb': ['0xae'], 'scasw': ['0x66', '0xaf'], 'scasd': ['0xaf'],
    'xlat': ['0xd7'], 'xlatb': ['0xd7'],
}
REP_BYTE = {'rep': '0xf3', 'repe': '0xf3', 'repz': '0xf3', 'repne': '0xf2', 'repnz': '0xf2'}


class Conv(Exception):
    pass


class Op:
    def __init__(self, kind, text, size=None, reg=None, indirect=False):
        self.kind = kind          # 'reg' | 'imm' | 'mem'
        self.text = text          # AT&T text
        self.size = size          # 'b' | 'w' | 'l' | None
        self.reg = reg            # lowercase register name (kind 'reg')
        self.indirect = indirect  # memory via [..] or PTR (valid for * branches)


def rsize(r):
    if r in REG8:
        return 'b'
    if r in REG16:
        return 'w'
    if r in REG32:
        return 'l'
    return None


def split_code_comment(line):
    """Split at the first '#' outside strings and character literals."""
    i, n, in_str = 0, len(line), False
    while i < n:
        c = line[i]
        if in_str:
            if c == '"':
                in_str = False
            i += 1
            continue
        if c == '"':
            in_str = True
            i += 1
            continue
        if c == "'" and i + 2 < n and line[i + 2] == "'":
            i += 3
            continue
        if c == '#':
            return line[:i], line[i:]
        i += 1
    return line, ''


def conv_chars(s):
    """Replace 'x' character literals (outside strings) with decimal codes."""
    out, i, in_str = [], 0, False
    while i < len(s):
        c = s[i]
        if in_str:
            out.append(c)
            if c == '"':
                in_str = False
            i += 1
            continue
        if c == '"':
            in_str = True
            out.append(c)
            i += 1
            continue
        if c == "'" and i + 2 < len(s) and s[i + 2] == "'":
            out.append(str(ord(s[i + 1])))
            i += 3
            continue
        out.append(c)
        i += 1
    return ''.join(out)


CONSTVAL = {}


def const_value(text):
    """Value of a constant expression (numbers, octal/decimal literals, .set constants)."""
    def ident(m):
        name = m.group(0)
        if name not in CONSTVAL:
            raise Conv('not a constant: ' + name)
        return '(%d)' % CONSTVAL[name]
    e = re.sub(r'(?<![\w$])[A-Za-z_][A-Za-z_0-9]*', ident, text)
    e = re.sub(r'(?<![\w.])0([0-7]+)\b', lambda m: str(int(m.group(1), 8)), e)
    e = e.replace('/', '//')
    if re.search(r'[^0-9xa-fA-F()+\-*//%<>&|^~ \t]', e):
        raise Conv('unsupported constant expression: ' + text)
    return eval(e, {'__builtins__': {}}, {})


def expr(t):
    t = conv_chars(t).strip()
    if '(' in t:
        t = str(const_value(t))
    return re.sub(r'(?i)\boffset\s+', '', t)


GLOBAL_CONSTS = {}


def note_const(name, val):
    """Record a constant's value for the rest of the file (GAS uses the value in effect at each use)."""
    try:
        CONSTVAL[name] = const_value(val)
    except (Conv, SyntaxError, NameError, ZeroDivisionError):
        CONSTVAL.pop(name, None)


def directive_expr(rest):
    """Directive operands: evaluate parenthesised constants in .set/= and data lists."""
    dn = rest.split(None, 1)[0].lower()
    arg = rest[len(dn):].strip()
    if dn in ('.set', '.equ'):
        name, _, val = arg.partition(',')
        return dn + ' ' + name.strip() + ', ' + expr(val)
    if dn in ('.byte', '.long', '.word', '.short'):
        return dn + ' ' + ', '.join(expr(x) for x in split_ops(conv_chars(arg)))
    return conv_chars(rest)


SYMRE = re.compile(r'(?<![\w$])[A-Za-z_][A-Za-z_0-9@]*')


def syms_in(e):
    return [m for m in SYMRE.findall(e) if m.lower() not in KEYWORDS]


def split_ops(s):
    ops, depth, cur = [], 0, ''
    for c in s:
        if c == '[':
            depth += 1
        elif c == ']':
            depth -= 1
        if c == ',' and depth == 0:
            ops.append(cur.strip())
            cur = ''
        else:
            cur += c
    if cur.strip():
        ops.append(cur.strip())
    return ops


def build_symbols(files):
    """Return {name: 'var' | 'const'} for every symbol defined in the port."""
    kind = {}
    defs = {}
    for f in files:
        for raw in open(f, encoding='latin-1'):
            code, _ = split_code_comment(raw.rstrip('\n'))
            code = code.strip()
            m = re.match(r'^((?:\d+|[A-Za-z_]\w*))\s*:', code)
            if m and not m.group(1).isdigit():
                kind[m.group(1)] = 'var'
            m = re.match(r'^\.(?:set|equ)\s+([A-Za-z_]\w*)\s*,\s*(.*)$', code)
            if m:
                defs.setdefault(m.group(1), []).append(m.group(2))
            m = re.match(r'^([A-Za-z_]\w*)\s*=\s*(.*)$', code)
            if m:
                defs.setdefault(m.group(1), []).append(m.group(2))
            m = re.match(r'^\.(?:lcomm|comm)\s+([A-Za-z_]\w*)', code)
            if m:
                kind[m.group(1)] = 'var'
            m = re.match(r'^\.extern\s+(.*)$', code)
            if m:
                for name in m.group(1).split(','):
                    kind[name.strip()] = 'var'
    # A name defined by .set/= is 'const' only if every symbol in its expression is const.
    changed = True
    while changed:
        changed = False
        for name, exprs in defs.items():
            if name in kind:
                continue
            verdicts = set()
            for e in exprs:
                refs = syms_in(conv_chars(e))
                if not refs:
                    verdicts.add('const')
                elif all(r in kind for r in refs):
                    verdicts.add('var' if any(kind[r] == 'var' for r in refs) else 'const')
                else:
                    verdicts.add(None)
            if len(verdicts) == 1 and None not in verdicts:
                kind[name] = verdicts.pop()
                changed = True
    # Constant values, for the expression evaluator. Conflicting redefinitions are reported.
    conflicts = set()
    for _ in range(20):
        for name, exprs in defs.items():
            if kind.get(name) != 'const':
                continue
            for e in exprs:
                try:
                    v = const_value(e)
                except (Conv, SyntaxError, NameError, ZeroDivisionError):
                    continue
                if name in CONSTVAL and CONSTVAL[name] != v:
                    conflicts.add(name)
                CONSTVAL.setdefault(name, v)
    CONFLICTS.update(conflicts)
    for name in CONFLICTS:
        CONSTVAL.pop(name, None)
    GLOBAL_CONSTS.update(CONSTVAL)
    return kind


CONFLICTS = set()


def reloc(syms, e):
    return any(syms.get(s, 'var') == 'var' for s in syms_in(e))


def mem_att(t, syms):
    m = re.match(r'^(.*?)\[(.*)\]\s*$', t)
    if not m:
        raise Conv('malformed memory operand')
    pre = expr(m.group(1)) if m.group(1).strip() else ''
    inner = re.sub(r'\s+', '', conv_chars(m.group(2)))
    base, index, disp = None, None, ''
    for sign, body in re.findall(r'([+-]?)([^+-]+)', inner):
        parts = body.split('*')
        r = parts[0].lower()
        if r in REGS and r not in REG32:
            raise Conv('16/8-bit register in address: ' + body)
        if r in REG32 and sign != '-':
            scale = int(parts[1]) if len(parts) == 2 else 1
            if len(parts) == 2 or base is not None:
                if index is not None:
                    raise Conv('two index registers')
                index = (r, scale)
            else:
                base = r
        else:
            if r in REGS:
                raise Conv('register subtracted: ' + body)
            if sign == '-' or (sign == '+' and disp != ''):
                disp += sign + body
            else:
                disp += body
    if pre and disp:
        d = pre + ('' if disp[0] in '+-' else '+') + disp
    else:
        d = pre or disp
    if base is None and index is None:
        return d
    s = d + '('
    if base:
        s += '%' + base
    if index:
        s += ',%%%s,%d' % index
    return s + ')'


def parse_operand(t, syms):
    t = t.strip()
    size = None
    ptr = False
    m = re.match(r'^(BYTE|WORD|DWORD)\s+PTR\s+(.*)$', t, re.I)
    if m:
        size = PTRSZ[m.group(1).upper()]
        ptr = True
        t = m.group(2).strip()
    tl = t.lower()
    if tl in REGS:
        return Op('reg', '%' + tl, rsize(tl), reg=tl)
    if tl in SEGS:
        raise Conv('segment register')
    m = re.match(r'^offset\s+(.*)$', t, re.I)
    if m:
        return Op('imm', '$' + expr(m.group(1)))
    if '[' in t:
        return Op('mem', mem_att(t, syms), size, indirect=True)
    if re.fullmatch(r'\d+[fb]', t):
        return Op('mem', resolve_num(t), size)
    e = expr(t)
    if ptr:
        return Op('mem', e, size, indirect=True)
    if reloc(syms, e):
        return Op('mem', e, None)
    return Op('imm', '$' + e)


def opsize(ops, mn):
    sizes = {o.size for o in ops if o.size}
    if len(sizes) > 1:
        raise Conv('conflicting operand sizes')
    if sizes:
        return sizes.pop()
    if mn in ('push', 'pop'):
        return 'l'
    raise Conv('no operand size for ' + mn)


def convert_insn(mn, opt, syms):
    ops = [parse_operand(t, syms) for t in opt]
    if mn in ('jmp', 'call'):
        if len(ops) != 1:
            raise Conv('branch arity')
        o = ops[0]
        if o.kind == 'reg':
            return mn, ['*' + o.text]
        if o.kind == 'mem':
            return mn, [('*' if o.indirect else '') + o.text]
        raise Conv('branch to immediate')
    if mn.startswith('j') or mn.startswith('loop'):
        if len(ops) != 1 or ops[0].kind != 'mem' or ops[0].indirect:
            raise Conv('conditional branch operand')
        return mn, [ops[0].text]
    if mn == 'lea':
        if len(ops) != 2 or ops[0].kind != 'reg' or ops[1].kind != 'mem':
            raise Conv('lea operands')
        return 'leal', [ops[1].text, ops[0].text]
    if mn in ('movzx', 'movsx'):
        if len(ops) != 2 or ops[0].kind != 'reg' or ops[1].kind not in ('reg', 'mem'):
            raise Conv('movzx operands')
        src_sz = ops[1].size
        if src_sz is None:
            raise Conv('movzx source size')
        base = 'movz' if mn == 'movzx' else 'movs'
        return base + src_sz + ops[0].size, [ops[1].text, ops[0].text]
    if mn == 'imul' and len(ops) == 3:
        d, s, i = ops
        if d.kind != 'reg' or i.kind != 'imm':
            raise Conv('imul 3-operand form')
        return 'imul' + d.size, [i.text, s.text, d.text]
    if mn in SHIFTS:
        if len(ops) != 2:
            raise Conv('shift arity')
        d, c = ops
        sz = opsize([d], mn)
        if c.kind == 'reg' and c.reg == 'cl':
            return mn + sz, ['%cl', d.text]
        if c.kind == 'imm' and c.text == '$1':
            return mn + sz, [d.text]
        if c.kind == 'imm':
            return mn + sz, [c.text, d.text]
        raise Conv('shift count')
    if mn in SUFFIXED:
        sz = opsize(ops, mn)
        if len(ops) == 1:
            return mn + sz, [ops[0].text]
        if len(ops) == 2:
            return mn + sz, [ops[1].text, ops[0].text]
        raise Conv('arity for ' + mn)
    # Anything else with operands is an immediate-only form (ret N, aam N).
    if all(o.kind == 'imm' for o in ops):
        return mn, [o.text for o in ops]
    raise Conv('unhandled mnemonic ' + mn)


CTX = {'defs': {}, 'line': 0, 'prefix': ''}


def num_defs(lines, prefix):
    """Numeric local labels ('1:') get unique names, so 1f / 1b can be resolved."""
    defs, count = {}, {}
    for i, l in enumerate(lines):
        code, _ = split_code_comment(l)
        m = re.match(r'^\s*(\d+):', code)
        if m:
            n = m.group(1)
            count[n] = count.get(n, 0) + 1
            defs.setdefault(n, []).append((i, '.L%sNL%s_%d' % (prefix, n, count[n])))
    return defs


def resolve_num(ref):
    n, d = ref[:-1], ref[-1]
    lst = CTX['defs'].get(n, [])
    i = CTX['line']
    if d == 'f':
        c = [name for (j, name) in lst if j > i]
        if not c:
            raise Conv('no forward label ' + ref)
        return c[0]
    c = [name for (j, name) in lst if j < i]
    if not c:
        raise Conv('no backward label ' + ref)
    return c[-1]


def conv_line(line, syms):
    s = line.lstrip()
    if s.startswith('#'):
        # GNU as 1.38 reads '#' followed by optional blanks and a digit as a line-number marker,
        # wherever it starts the line (indented or not). The marker silently drops the section's
        # code, so a comment whose text starts with a digit gets a '-' after the '#'.
        if re.match(r'\s*\d', s[1:]):
            return '\t#-' + s[1:]
        return '\t' + s if not line.startswith(('\t', ' ')) else line
    code, comment = split_code_comment(line)
    tail = ('\t' + comment) if comment else ''
    if code.strip() == '':
        return line
    indent = re.match(r'\s*', code).group(0)
    rest = code.strip()
    label = ''
    m = re.match(r'^((?:\d+|[A-Za-z_]\w*):)\s*(.*)$', rest)
    if m:
        name = m.group(1)[:-1]
        if name.isdigit():
            name = [nm for (j, nm) in CTX['defs'][name] if j == CTX['line']][0]
        label, rest = name + ':\t', m.group(2).strip()
    if rest == '':
        return (indent + label.rstrip('\t') + tail) if label else line
    if rest.startswith('.'):
        parts = rest.split(None, 1)
        dn = parts[0].lower()
        arg = parts[1] if len(parts) > 1 else ''
        if dn in ('.intel_syntax', '.att_syntax'):
            return (indent + label + tail.lstrip('\t')).rstrip() if (label or tail) else ''
        if dn == '.globl' and ',' in arg:
            # GNU as 1.38 exports only the first name of a multi-name .globl: one directive per name
            return '\n'.join(indent + label + '.globl ' + n.strip() + tail for n in arg.split(','))
        if dn == '.extern':
            return indent + tail.lstrip('\t') if tail else ''
        if dn == '.bss':
            return indent + label + '.data' + tail
        if dn == '.skip':
            rest = '.space' + rest[len(dn):]
        if dn == '.section':
            sec = arg.split(',')[0].strip()
            mapped = {'.text': '.text', '.data': '.data', '.bss': '.data', '.rodata': '.data'}
            if sec not in mapped:
                raise Conv('section ' + sec)
            return indent + label + mapped[sec] + tail
        out = directive_expr(rest)
        if dn in ('.set', '.equ'):
            name, _, val = rest[len(dn):].strip().partition(',')
            note_const(name.strip(), val)
        return indent + label + out + tail
    m = re.match(r'^([A-Za-z_]\w*)\s*=\s*(.*)$', rest)
    if m:
        out = indent + label + m.group(1) + ' = ' + expr(m.group(2)) + tail
        note_const(m.group(1), m.group(2))
        return out
    parts = rest.split(None, 1)
    mn = parts[0].lower()
    argstr = parts[1] if len(parts) > 1 else ''
    if mn in PREFIXES:
        inner = argstr.strip().lower()
        if inner not in STRING_OPS:
            raise Conv('prefix on non-string op: ' + argstr)
        ops_b = STRING_OPS[inner]
        if mn in REP_BYTE:
            bs = ops_b[:-1] + [REP_BYTE[mn]] + ops_b[-1:] if len(ops_b) == 2 else [REP_BYTE[mn]] + ops_b
        return indent + label + '.byte\t' + ', '.join(bs) + tail
    opt = split_ops(conv_chars(argstr))
    if not opt and mn in STRING_OPS:
        return indent + label + '.byte\t' + ', '.join(STRING_OPS[mn]) + tail
    if not opt:
        new = RENAME0.get(mn, mn)
        return indent + label + new + tail
    name, ops = convert_insn(mn, opt, syms)
    return indent + label + name + '\t' + ', '.join(ops) + tail


INC_RE = re.compile(r'^\s*\.include\s+"([^"]+)"')


LNAMES = {}     # exported names that start with 'L' -> converted name (see rename_lnames)


def exported_l_names(files):
    """Names in .globl directives that start with 'L'. GNU as 1.38 (a.out) treats any symbol
    beginning with 'L' as a local label and does not write it to the object, even with .globl,
    so another file cannot link to it. These names are renamed in the converted tree."""
    names = set()
    for f in files:
        if not f.endswith('.S'):
            continue
        for line in open(f, encoding='latin-1'):
            m = re.match(r'^\s*\.globl\s+(.*)$', line)
            if m:
                for n in re.split(r'[,\s]+', split_code_comment(m.group(1))[0].strip()):
                    if n.startswith('L'):
                        names.add(n)
    return names


def check_lnames(names, files):
    """Each new name must not already occur in the sources (a clash would merge two symbols)."""
    text = ''.join(open(f, encoding='latin-1').read() for f in files)
    for n in sorted(names):
        new = 'XL' + n
        if re.search(r'(?<![A-Za-z0-9_])' + new + r'(?![A-Za-z0-9_])', text):
            raise SystemExit('rename clash: %s already used' % new)
        LNAMES[n] = new


def rename_lnames(line):
    """Rename whole identifiers in LNAMES outside double-quoted strings."""
    if not LNAMES or not re.search(r'L', line):
        return line
    parts = line.split('"')
    for k in range(0, len(parts), 2):
        parts[k] = re.sub(r'(?<![A-Za-z0-9_.])([A-Za-z_]\w*)(?![A-Za-z0-9_])',
                          lambda m: LNAMES.get(m.group(1), m.group(1)), parts[k])
    return '"'.join(parts)


REPT_RE = re.compile(r'^\s*\.rept\s+(\d+)\s*(#.*)?$')
ENDR_RE = re.compile(r'^\s*\.endr\b')


def expand_rept(lines, name, errors):
    """GNU as 1.38 has no .rept: repeat each .rept body N times. Returns (line number, line) pairs,
    so error messages keep the source line numbers. A body that defines a label is an error,
    because a repeated label would be defined twice."""
    out = []
    i = 0
    while i < len(lines):
        m = REPT_RE.match(lines[i])
        if not m:
            out.append((i, lines[i]))
            i += 1
            continue
        j = i + 1
        while j < len(lines) and not ENDR_RE.match(lines[j]):
            j += 1
        if j >= len(lines):
            errors.append('%s:%d: .rept without .endr' % (name, i + 1))
            break
        body = [(k, lines[k]) for k in range(i + 1, j)]
        if any(re.match(r'^\s*(\d+|[A-Za-z_]\w*):', l) for _, l in body):
            errors.append('%s:%d: .rept body defines a label (cannot repeat)' % (name, i + 1))
        else:
            for _ in range(int(m.group(1))):
                out.extend(body)
        i = j + 1
    return out


def convert_file(path, syms, errors, inline_dir=None, depth=0):
    """Convert one file. With inline_dir, .include lines are replaced by the converted include."""
    name = os.path.basename(path)
    base = re.sub(r'\W', '_', os.path.splitext(name)[0])
    lines = open(path, encoding='latin-1').read().split('\n')
    CTX['defs'] = num_defs(lines, base + '_') if depth == 0 else CTX['defs']
    if depth == 0:
        CONSTVAL.clear()
        CONSTVAL.update(GLOBAL_CONSTS)
    res = []
    for i, line in expand_rept(lines, name, errors):
        CTX['line'] = i
        m = INC_RE.match(line) if inline_dir else None
        if m and depth < 4:
            res.append('# .include "%s" inlined for GNU as 1.38' % m.group(1))
            saved = CTX['defs']
            res.extend(convert_file(os.path.join(os.path.dirname(path), m.group(1)), syms, errors,
                                    inline_dir, depth + 1))
            CTX['defs'] = saved
            continue
        try:
            res.append(rename_lnames(conv_line(line, syms)))
        except Conv as ex:
            errors.append('%s:%d: %s: %s' % (name, i + 1, ex, line.strip()))
            res.append('# CONVERT-ERROR ' + line.strip())
    return res


def main():
    src, out = sys.argv[1], sys.argv[2]
    args = sys.argv[3:]
    inline = '--inline' in args
    memsize = None
    if '--memsize' in args:
        memsize = args[args.index('--memsize') + 1]
    files = sorted(glob.glob(os.path.join(src, '*.S')) + glob.glob(os.path.join(src, '*.inc')))
    syms = build_symbols(files)
    check_lnames(exported_l_names(files), files)
    os.makedirs(out, exist_ok=True)
    errors = []
    for f in files:
        name = os.path.basename(f)
        if name.endswith('.inc') and inline:
            continue            # included (inlined) by the .S files that use it
        res = convert_file(f, syms, errors, inline_dir=src if inline else None)
        if memsize and any(re.search(r'\bMEMSIZE\b', l) for l in res):
            # the GNU as 1.38 build has no --defsym: define the workspace size in the file itself
            res.insert(0, '\t.set MEMSIZE, %s' % memsize)
        with open(os.path.join(out, name), 'w', encoding='latin-1') as fh:
            fh.write('\n'.join(res))
    if inline:
        for f in files:
            if f.endswith('.inc'):
                name = os.path.basename(f)
                with open(os.path.join(out, name), 'w', encoding='latin-1') as fh:
                    fh.write('\n'.join(convert_file(f, syms, errors)))
    print('symbols: %d (var %d, const %d)' % (len(syms), sum(v == 'var' for v in syms.values()),
                                              sum(v == 'const' for v in syms.values())))
    print('files: %d, errors: %d' % (len(files), len(errors)))
    if CONFLICTS:
        print('constants with conflicting values in different files: %s' % ' '.join(sorted(CONFLICTS)))
    for e in errors[:200]:
        print(e)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
