# R3 independent shell comparisons

Original RC6 shell blocks are extracted without algorithm edits, then converted
using the same syntax-only rules as R2 and assembled with m68k GAS 1.38. The
original routines execute in M68000 mode with their documented internal register
inputs. Only the oracle maps the old physical bitmap address 0x20000. The native
routines take explicit cdecl pointers and draw into test-owned memory instead.

Native instructions come from actual i386 GAS 1.38 and the historical i386
linker's final a.out image. R2 `oracle_pair.py` is reused unchanged. No C gameplay
or generic register-bank translation is involved.

`make shell-check` passed:

- 2,048 original text and viewport-clear cases: full 32KB packed bitmap equality,
  preserved dashboard, default and explicit X, right/bottom rejection, existing
  pixel OR behavior, original ASCII glyphs, native ABI and guard bytes.
- 4,096 fixed-width unsigned decimal cases: 32-bit values, 1..10 digits, exact
  written bytes, returned quotient and guards.
- 2,048 save checksum and copy/restore cases: EQA1/EQA2 checksum lengths, original
  rotate/add order and wrap, all six raw payload blocks and guards. This verifies
  leaf routines, not full save validation/menu/file orchestration.

Text/decimal SHA256: `5af01e98d24596d90d923a70c41af86882c6fdb14a28af4a6adf2ea95d83356e`.
Combined SHA256: `e0ef5da99b76c048d46d4141da88541e5a7decec9e2e9597f3c8dd007176613a`.
Fixed input seed: 0x3865e11. Exact construction: tools/diff-shell-text.py.
Log: R3-SHELL-TEXT-ORACLE.log.

The complete R3 suites now execute the full original shell and the native shell.
See `R3-FULL-SHELL-TEST.log` for reproducible hashes and `R3-VALIDATION.md` for
scope and limitations. `R3-FROZEN-REGRESSIONS.log` records the unchanged 33 R2
fixture suites, platform tests, four R2 oracle groups and the above leaf suites.
The title-load source defect is deliberately exposed, not silently repaired.
