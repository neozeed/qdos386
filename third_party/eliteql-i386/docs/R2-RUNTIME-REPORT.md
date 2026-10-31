# R2 runtime report — incomplete checkpoint

Build date: 2026-10-30. Baseline: supplied EliteQL 1.0 RC6 and frozen Elite386 R1.
Working branch: `eliteql-i386-r2-assembly-completion`.

Native code is assembled by the actual bundled host64 GAS 1.38. The historical
final link uses the bundled host64 binutils 1.94 i386 linker, from the supplied
I386VM-R1-GCC140-GAS138-AOUT source archive. The original QDOS386 R14A/HACK103 tree
was used only as a reference and was not modified.

The source inventory accounts for 45 original .s files and 2,535 labels.
44 files have native counterparts; `shell.s` has none. Native code includes
144 unique exported names (data as well as functions; not a function count).
The source manifest includes original hashes, data directives, call references,
cross-file references and observed hardware dependencies.

## Observed results

- 33 fixture suites pass, retaining all eleven R1 suites.
- File/queue/sound platform sequence tests pass with recording adapters.
- Independent m68k comparisons pass for 12,288 movement cases, 2,048 raster
  frames, 2,048 ship renders, 512 circles, 1,024 dust scenarios over three
  steps, 1,024 cloud-point cases and 64 complete galaxy charts.
- Differential tests execute the historical linker's final a.out image.
- Benchmark/development alternatives assemble with actual GAS 1.38.
- A fresh source-only extraction rebuilds the assemblers/linker and reruns
  the same fixture/platform/oracle/alternative targets. See R2-FRESH-VALIDATION.log.
- Fresh-build byte comparison: all 47 assembled objects (including alternatives
  and test entry) and the final historical a.out image are identical. All 45
  original assembly files match the supplied RC6 tree byte-for-byte.
- Per-object source-label checks reject silently truncated GAS output. This
  catches the historical assembler's interpretation of numeric-leading comments
  as line directives; source comments were corrected rather than ignoring it.

CPU execution is under Unicorn. No game execution inside QDOS386 is claimed.
The current `core-headless` entry is a link-test return, not a game main loop.

## Handoff gates

| Gate | Status |
|---|---|
| 0 — source inventory | All labels classified; full shell/palette behavior audit unfinished |
| 1 — all non-rendering gameplay and independent comparison | INCOMPLETE: shell state/control routines absent; individual oracle coverage partial |
| 2 — all drawing | INCOMPLETE: core raster/geometry/effects present; original shell text/menu drawing absent |
| 3 — entire native control flow with headless adapter | NOT MET |
| 4 — full behavioral scenarios | NOT MET; only explicitly listed suites pass |
| 5 — complete assembly release | NOT MET; this is a reproducible work-in-progress checkpoint |

No missing shell behavior is replaced by a stub or C implementation. No new
features, models, palettes, controls or physics were introduced. The remaining
work is named rather than treated as OS integration. The R1 checkpoint is not
changed or relabelled as complete.
