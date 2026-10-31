#!/usr/bin/env python3
"""Generate the data part of src/ibmres.S from the original IBMRES.ASM.

Only the macro-driven data region is processed: everything before the line
"SUBTTL  CRUNCH code ...". The 8086 code region is hand-translated and appended from
tools/ibmres_tail.S.

Macro semantics (from IBMRES.H / BINTRP.H):
  T X      QQ=QQ+1; $X=QQ                     (no data)
  R X      DW X; QQ=QQ+1; $X=QQ               (2-byte dispatch address -> .long here)
  R2 X,Y   DW Y; QQ=QQ+1; $X=QQ
  Q X      keyword text: characters 2..n of X, last char with bit 7 set, then token $X
  QF X     as Q, but the token byte is $X-128 (function tokens)
  ADR X    DW X                               (.long here)
  DB / DW  as in MASM (DW emits .long; widened pointers)
  label:   label
  NAME=expr  or  QQ=expr  assignments: evaluated here and emitted as numeric .set

Symbol names: '$' becomes '_' for GAS. The first character of a Q keyword is not
emitted, because the alphabetic table it sits in is indexed by that character.
"""
import re, sys, os

SRC = sys.argv[1]
OUT = sys.argv[2]
TAIL = sys.argv[3]

raw = open(SRC, 'rb').read().decode('latin-1').replace('\r', '')
lines = raw.split('\n')

sym = {}          # MASM symbol -> int
public = set()    # names declared PUBLIC
out = []          # GAS lines
pending_bytes = []  # bytes accumulated for a .byte line
stop_at = None
for i, l in enumerate(lines):
    if re.match(r'\s*SUBTTL\s+CRUNCH code', l):
        stop_at = i
        break

def gas_name(n):
    return n.replace('$', '_').replace('?', 'Q_')

def strip_comment(l):
    # comments start with ';' outside of a string literal
    out_s = ''
    q = None
    for ch in l:
        if q:
            out_s += ch
            if ch == q:
                q = None
        elif ch in '"\'':
            q = ch
            out_s += ch
        elif ch == ';':
            break
        else:
            out_s += ch
    return out_s.rstrip()

def ev(expr):
    """Evaluate a MASM expression (numbers, symbols, string literals, ops) to int."""
    e = expr.strip()
    # string literal "x" or 'x'  -> ord
    def repl_str(m):
        return str(ord(m.group(1)))
    e = re.sub(r'"(.)"', repl_str, e)
    e = re.sub(r"'(.)'", repl_str, e)
    e = re.sub(r'\bOFFSET\s+', '', e)
    e = re.sub(r'\bLOW\s+', '', e)
    e = re.sub(r'\bAND\b', '&', e)
    e = re.sub(r'\bOR\b', '|', e)
    # octal number with O suffix, decimal with D suffix
    e = re.sub(r'\b([0-7]+)O\b', lambda m: str(int(m.group(1), 8)), e)
    e = re.sub(r'\b([0-9]+)D\b', lambda m: str(int(m.group(1), 10)), e)
    def repl_sym(m):
        name = m.group(0)
        if name in sym:
            return '(' + str(sym[name]) + ')'
        raise KeyError('undefined symbol in expression: ' + name + ' in ' + expr)
    e = re.sub(r'[$A-Za-z_?][$A-Za-z0-9_?]*', repl_sym, e)
    return eval(e, {'__builtins__': {}}, {})

def flush():
    global pending_bytes
    if pending_bytes:
        out.append('\t.byte ' + ', '.join(str(b & 0xff) for b in pending_bytes))
        pending_bytes = []

def emit_byte(v):
    pending_bytes.append(v)

def emit_long(name_or_expr):
    flush()
    out.append('\t.long ' + gas_name(name_or_expr))

def do_Q(name, is_func):
    # Q/QF X: characters 2..n of X, last with bit 7, then token value
    key = '$' + name
    if key not in sym:
        raise KeyError('Q/QF before token defined: ' + name)
    chars = name[1:]
    if len(chars) == 0:
        raise ValueError('Q keyword too short: ' + name)
    for c in chars[:-1]:
        emit_byte(ord(c))
    emit_byte(ord(chars[-1]) | 0x80)
    tok = sym[key] - (128 if is_func else 0)
    emit_byte(tok)

ASSIGN = re.compile(r'^\s*([$A-Za-z_?][$A-Za-z0-9_?]*)\s*=\s*(.+)$')
LABEL = re.compile(r'^\s*([$A-Za-z_?][$A-Za-z0-9_?]*)\s*:\s*$')
TOK = re.compile(r'^\s*(T|R|R2|Q|QF|ADR|DB|DW|PUBLIC|EXTRN|DSEG|ENDS|CSEG|SEGMENT|ASSUME|END|PAGE|TITLE|SUBTTL|\.RADIX|\.SALL|\.XLIST|\.LIST|IF1|ENDIF|INCLUDE)\b\s*(.*)$')

def tokenise_args(s):
    return [a.strip() for a in s.split(',')] if s.strip() else []

# Pass 1 and 2 combined: MASM symbols are defined before use in this file, except for
# DUMMY (defined repeatedly) and the NUMGFN group, which uses values defined earlier.
in_comment = False
for i, raw_line in enumerate(lines[:stop_at]):
    l = strip_comment(raw_line)
    s = l.strip()
    if s.startswith('COMMENT') and '*' in s:
        in_comment = True
        continue
    if in_comment:
        if s.startswith('*'):
            in_comment = False
        continue
    if not s or s in ('^Z', '\x1a') or s.startswith('\x00'):
        continue
    if s.startswith('.') and s.upper().startswith('.RADIX'):
        continue
    if s.upper().startswith(('CSEG', 'DSEG', 'ASSUME', 'INCLUDE', 'SUBTTL', 'TITLE', 'PAGE', '.SALL', 'END', 'EXTRN', 'IF1', 'ENDIF', 'ENDS', 'SEGMENT')):
        if s.upper().startswith('EXTRN'):
            continue
        continue
    m = re.match(r'^PUBLIC\s+(.*)$', s)
    if m:
        for n in tokenise_args(m.group(1)):
            public.add(n)
            out.append('\t.globl ' + gas_name(n))
        continue
    m = LABEL.match(s)
    if m:
        flush()
        out.append(gas_name(m.group(1)) + ':')
        continue
    # assignment: e.g. "QQ=QQ+1", "$KEY2B=0+QQ", "NUMCMD=QQ-$END+1"
    m = ASSIGN.match(s)
    if m and not re.match(r'^(R|R2|T|Q|QF|ADR|DB|DW)\b', s):
        name, expr = m.group(1), m.group(2)
        val = ev(expr)
        sym[name] = val
        if name.startswith('$') or name in public or name == 'QQ':
            pass
        if name != 'QQ':
            out.append('\t.set %s, %d' % (gas_name(name), val))
            if name in public:
                out.append('\t.globl ' + gas_name(name))
        continue
    m = TOK.match(s)
    if not m:
        raise SystemExit('UNHANDLED LINE %d: %r' % (i + 1, raw_line))
    op = m.group(1)
    args = m.group(2)
    if op == 'T':
        name = args.strip()
        sym['QQ'] = sym['QQ'] + 1
        sym['$' + name] = sym['QQ']
        out.append('\t.set %s, %d' % (gas_name('$' + name), sym['QQ']))
        out.append('\t.globl ' + gas_name('$' + name))
    elif op == 'R':
        name = args.strip()
        flush()
        out.append('\t.long ' + gas_name(name))
        sym['QQ'] = sym['QQ'] + 1
        sym['$' + name] = sym['QQ']
        out.append('\t.set %s, %d' % (gas_name('$' + name), sym['QQ']))
        out.append('\t.globl ' + gas_name('$' + name))
    elif op == 'R2':
        a = tokenise_args(args)
        flush()
        out.append('\t.long ' + gas_name(a[1]))
        sym['QQ'] = sym['QQ'] + 1
        sym['$' + a[0]] = sym['QQ']
        out.append('\t.set %s, %d' % (gas_name('$' + a[0]), sym['QQ']))
        out.append('\t.globl ' + gas_name('$' + a[0]))
    elif op in ('Q', 'QF'):
        name = args.strip()
        do_Q(name, op == 'QF')
    elif op == 'ADR':
        flush()
        out.append('\t.long ' + gas_name(args.strip()))
    elif op == 'DB':
        for a in tokenise_args(args):
            if a.startswith('"') and a.endswith('"') and len(a) >= 3 and '+' not in a:
                for c in a[1:-1]:
                    emit_byte(ord(c))
            else:
                # numeric or expression, possibly with a string-literal first term
                emit_byte(ev(a))
    elif op == 'DW':
        for a in tokenise_args(args):
            a2 = a.strip()
            if re.match(r'^OFFSET\s+', a2):
                a2 = re.sub(r'^OFFSET\s+', '', a2)
            if re.match(r'^[$A-Za-z_?][$A-Za-z0-9_?]*$', a2) and a2 not in sym:
                flush()
                out.append('\t.long ' + gas_name(a2))
            else:
                flush()
                out.append('\t.long %d' % ev(a2))
    else:
        raise SystemExit('UNHANDLED OP %d: %r' % (i + 1, raw_line))

flush()

# Statement dispatch and function dispatch: sizes must be known to the code region.
header = [
    '# IBMRES.ASM -> ibmres.S (generated by tools/gen_ibmres.py, then hand-translated tail)',
    '# DO NOT EDIT THE DATA BELOW BY HAND: regenerate from the original.',
    '# Tables of addresses are widened: each DW entry is now 4 bytes (.long).',
    '\t.intel_syntax noprefix',
    '\t.include "oem.inc"',
    '\t.include "gio86u.inc"',
    '\t.include "msdosu.inc"',
    '\t.section .data',
]
body = out
with open(OUT, 'w') as f:
    f.write('\n'.join(header + body) + '\n')
    f.write(open(TAIL).read())
print('wrote', OUT, 'lines', len(body), 'symbols', len(sym))
