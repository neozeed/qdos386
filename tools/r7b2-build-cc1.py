#!/usr/bin/env python3
"""Build the archive-pinned GCC 1.40 host source checkpoint, entirely in /tmp.
Usage: python3 tools/r7b2-build-cc1.py SOURCE_CHECKPOINT NEW_OUTPUT_DIRECTORY
SOURCE_CHECKPOINT must contain gcc-1.40/ and generated/. No downloads/install.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

source, output = (Path(x).resolve() for x in sys.argv[1:])
if not str(source).startswith('/tmp/') or not str(output).startswith('/tmp/'):
    raise SystemExit('Both source copy and output must be under /tmp')
output.mkdir(exist_ok=False)
(output / 'scratch').mkdir()
env = dict(os.environ, TMPDIR=str(output / 'scratch'))
s, g = source / 'gcc-1.40', source / 'generated'
flags = ['-std=gnu89', '-w', '-O0', '-g', '-DLEGACY_HOST64',
         '-DTARGET_CPU_I386', '-I' + str(g), '-I' + str(s), '-I' + str(s / 'config')]
names = ('c-decl c-typeck c-conv toplev version tree print-tree stor-layout '
         'fold-const rtl rtlanal expr stmt expmed explow optabs varasm symout '
         'dbxout sdbout emit-rtl integrate jump cse loop flow stupid combine '
         'regclass local-alloc global-alloc reload reload1 caller-save final '
         'recog pragma obstack').split()
generated = 'c-tab insn-emt insn-xrt insn-out insn-pep insn-rcg'.split()
records = []


def run(label, args):
    with (output / (label + '.log')).open('wb') as log:
        result = subprocess.run(args, cwd=output, env=env, stdout=log,
                                stderr=subprocess.STDOUT)
    records.append({'argv': args, 'exit': result.returncode})
    (output / 'commands.json').write_text(json.dumps(records, indent=2) + '\n')
    if result.returncode:
        raise SystemExit(result.returncode)


run('host-version', ['gcc', '-v'])
for name in names + generated:
    cfile = (g if name in generated else s) / (name + '.c')
    run(name, ['gcc', *flags, '-c', str(cfile), '-o', str(output / (name + '.o'))])
run('link', ['gcc', *[str(output / (n + '.o')) for n in sorted(names + generated)],
             '-lm', '-o', str(output / 'cc1')])
run('version', [str(output / 'cc1'), '-version'])
receipt = {'compiler_sha256': hashlib.sha256((output / 'cc1').read_bytes()).hexdigest(),
           'sources': {str(p.relative_to(source)): hashlib.sha256(p.read_bytes()).hexdigest()
                       for root in [s, g] for p in sorted(root.rglob('*')) if p.is_file()}}
(output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(receipt['compiler_sha256'], output / 'cc1')
