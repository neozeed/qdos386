# R11C scope: floating-point conversion, formatting and selected libm

Branch `r11c`, created from `r11b` at `aa44be5`.

## Accepted baseline

- R11B is accepted as the completed development milestone.
- Archive `qdos386-R11B.tar.gz`, sha256 `d9e3d3865d8fe45bf812b4ddff60a95c03bbf1ccc8b8335184c8e5c4d5545964`,
  built from source commit `aa44be5`. It is preserved unchanged. Nothing under the
  R11B archive is regenerated or edited in R11C.
- R10 and R11A frozen artifacts are unchanged (`r10-fpu-frozen` 533ee57, `r10-vga-frozen` 9f3340a).

## Test fixtures retained (owner confirmation)

The owner confirmed that the two external fixtures may be included in the QDOS/386
project packages as frozen regression-test fixtures:

- `LGTEST4.aout` (3,576-check FPU and compiler-runtime suite). Kept with its exact
  sha256 (see `EXTERNAL-DEPENDENCIES.txt` in the archive). Its three known GCC 1.40
  `floatundidf` discrepancies stay documented as test results. They are not hidden
  and the suite is not edited to pass them.
- `FPJOB.aout` (8,601 bytes, sha256 `114ed367…`). Kept with its exact hash. It is
  never silently replaced by a rebuild (rebuilds of the available sources give
  9,472 and 11,415 bytes, neither matching). The provenance and reproducibility
  question is a separate investigation track that does not block R11C.

Third-party copyright and licence notices continue to apply. The provenance table
in `docs/R11B-LICENSE-PROVENANCE.md` still governs what is shipped. The confirmation
covers inclusion of these test artifacts only.

## Goals

1. Floating-point conversion: `double` and `float` conversion helpers the C89
   code needs, verified on Tiny386, i386VM and QEMU (scripted).
2. Formatting: `%f`, `%e`, `%g` in the stdio formatter, built on the conversions
   above, with correct rounding limits documented.
3. Selected libm: `frexp`, `ldexp`, then `floor`, then `pow` if needed. No complete
   libm.

Starting point: `docs/R11A-MATH-ROADMAP.md`. The C68 `libieee` routines are
reference only; implementations are written from the C89 standard and the x87
behaviour verified in R10.

## Non-goals

- No SuperBASIC integration.
- No file writes (FAT16 stays read-only).
- No change to frozen R10/R11A/R11B artifacts or to the user's Tiny386 binary.
- No QEMU-specific behaviour in the kernel.
- Scripted input stays labelled as scripted. The human QEMU session is recorded
  only as a user-reported screenshot.

## Known limits to carry forward

- Tiny386 rounding depends on the version-specific external patch
  (`fpu-round-nearest.patch`).
- FNSTENV tag word reads differ across Tiny386, i386VM and QEMU.
- The three `floatundidf` discrepancies in LGTEST4.

## Open items

- Phase plan and acceptance tests to be written before implementation.
- FPJOB provenance investigation: separate track, owner notified in the R11C report.
