#!/bin/sh
# Regenerate built-in X11 fonts and their kernel tables deterministically.
# Inputs: source/fonts/src/ (notices retained alongside the PCF sources).
# Outputs: source/fonts/{RECOVERY,CON8X16}.FNT and source/kernel/font_*.c.
# The separate optional GPL Minerva font stays under third_party/minerva/;
# no Minerva bitmap data is linked into the production kernel.
# Usage: sh source/tools/r13a-font/build-fonts.sh [OUTDIR]
# OUTDIR defaults to the repository root; an explicit directory mirrors its layout.
# MINERVA_ROM, when set, enables the existing source-versus-ROM cross-check.
set -eu
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
cd "$ROOT"
O=${1:-.}
mkdir -p "$O/source/fonts" "$O/source/kernel" "$O/third_party/minerva"
O=$(cd "$O" && pwd)
qfnt() { python3 "$ROOT/source/tools/r13a-font/qfnt.py" "$@"; }

qfnt pcf "$ROOT/source/fonts/src/koi6x9.pcf.gz" "$O/source/fonts/RECOVERY.FNT" \
  "Recovery 6x9 (PD)" ascii 0x20 0x7e 6 10
qfnt pcf "$ROOT/source/fonts/src/koi8x16.pcf.gz" "$O/source/fonts/CON8X16.FNT" \
  "Console 8x16" ascii 0x20 0x7e 8 16
qfnt c "$O/source/fonts/RECOVERY.FNT" font_recovery "$O/source/kernel/font_recovery.c"
qfnt c "$O/source/fonts/CON8X16.FNT" font_console "$O/source/kernel/font_console.c"

if [ -n "${MINERVA_ROM:-}" ]; then
  qfnt minerva "$ROOT/third_party/minerva/src/tb_f0.s" "$ROOT/third_party/minerva/src/tb_f1.s" \
    "$O/third_party/minerva/MINERVA.FNT" "$MINERVA_ROM"
else
  qfnt minerva "$ROOT/third_party/minerva/src/tb_f0.s" "$ROOT/third_party/minerva/src/tb_f1.s" \
    "$O/third_party/minerva/MINERVA.FNT"
fi

# Keep the established logical font names in generated provenance comments,
# independent of the repository's physical source/ prefix or output directory.
python3 - "$O" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1])
for font, table in [('RECOVERY.FNT', 'font_recovery.c'), ('CON8X16.FNT', 'font_console.c')]:
    path = out / 'source/kernel' / table
    data = path.read_bytes()
    prefix = ('from ' + str(out / 'source/fonts' / font)).encode()
    assert data.count(prefix) == 1, 'unexpected generated font provenance'
    path.write_bytes(data.replace(prefix, ('from fonts/' + font).encode(), 1))
PY
