# Patch note: GW-BASIC QDOS/386 Gates 0–4

Baseline: the tree handed over with the KCTX-FIX2 kernel (Gate 0 pass; `docs/GWBASIC-KCTX-FIX2-HANDBACK.md`).
The kernel was not modified and is not part of this package. This note lists what changed in the
port, the tests and the documents since that baseline. Full evidence is in `logs/` and `docs/`.

## Source changes (`src/`, `qdos/`)

| File | Change | Why |
|---|---|---|
| `src/bifile.S` | SAVE keeps the file-name pointer in `SAVNAME` across `CHRGTR` and `FILEND` (Gate 1). MERGE end handling fixed (earlier pass). | SAVE reported Device I/O Error because the pointer was clobbered by `CHRGTR`. |
| `src/gweval.S` | `ISFUN` funny-function path pushes the second argument as 2 bytes (`push si`), matching `PREAM2`. DEF FN, DBLDPC and FACDBL fixes (earlier pass). | Slicing corrupted the stack (`MID$`, `LEFT$`, `RIGHT$`). |
| `src/stage3stubs.S` | `FILGET` backs up the text pointer before the `#` test. `FINPRT` clears `PTRFIL`. | `PRINT A#` was rejected as Advanced Feature. After `LLIST`, the error message went to the printer and looped forever. |
| `src/math1.S` | A `#` after an exponent (`1E3#`) forces double precision and is consumed. | `1E3#` printed as single precision. |
| `src/math2.S` | `SNGINT` (INT for singles) rewritten for GNU as 1.38: normalising loop instead of `BSR`, explicit push and pop instead of `PUSHAD`/`POPAD`. | GNU as 1.38 has neither `BSR` nor `PUSHAD`/`POPAD`. The INT result for singles was also corrected (`int_edge`). |
| `src/gwmain_b2.S` | DATA search: the start-of-line sentinel (CR) is handled before `DATA`, and the line link is 4 bytes (QDOS layout). | `READ` always reported Out of DATA. |
| `src/fiveo.S`, `src/forent.inc` | Comments corrected (`WEND without WHILE`, `WHLSIZ`). | Documentation only. |
| `qdos/platform_qdos.S` | Changed during Gates 1–3. Its write-error mapping (`plat_wrerr`: kernel −80 → code 19 write-protected, −81 → disk full, root directory full → 4) and the file error codes in the header are the relevant parts. The exact diff from the handback baseline was not recorded; compare with the handback archive if needed. | Write errors reach the BASIC program as the messages in the save_* cases. |
| `qdos/repro/` | Spin programs moved to `evidence/alignment-control/`. | Evidence, not build input. |

`src/deferred/lprint.S` is kept but is not assembled by either build (it was never wired in).

## Build changes

- `Makefile`: the Win32 object rule now depends on `src/*.inc`. Before, changing an include file did not rebuild
  the objects that include it, so the Win32 binary used during Gates 2–3 was partly stale. It was rebuilt from
  clean; the clean build passes `make test` 42 of 42. The QDOS image rebuilt from the package is byte-identical
  to the one in it (`c7dd8abc…`).

## Test changes (`tests/`)

- `tests/cases/`: new shared cases `dp_var_print`, `read_data`, `llist_unsupported`, `int_edge` (Stage 4
  regressions), and `long_loops` (FOR to 16000, 16400, 20000, 200000; nested 100 × 1000; 20000 GOSUB
  calls). The WHILE/WEND regression cases `whl_*` were run unchanged. Win32: 42 of 42.
- `tests/qdos/cases/load_errors.{in,exp,qcfg}`: a QDOS replacement of the shared `load_errors` case.
  It differs only in the file name (`BADSTR.BAS`, since 9-character base names are not valid on the QDOS
  volume), with the same fixtures as `load_errors_83`. Expectations are unchanged.
- `tests/qdos/run_qdos.py`: a `settle N` directive in `NAME.qcfg` sets the wait after each typed line
  before its screen is captured (default 1 s). Needed for `long_loops`. Nothing else changed.
- `tests/coverage/`: the statement and function probe (`probe_matrix.py`, now relative to the tree) and
  the renderer (`matrix_to_md.py`). The old probe had hard-coded scratch paths.
- `tests/run_gw.py`: the output directory defaults to the system temp directory (it had a hard-coded scratch path);
  `GWTEST_OUT` overrides it. Its docstring now says which expectations were written by hand.
- `tests/historical/`: the Mandelbrot listing and licence (shipped), the Star Trek fetch and checks,
  and the preparation script that reproduces the two inputs that were run.

## Removed from the tree (not in the package)

- The stray root `a.out`.
- `qdos/repro/basic` (binary) and the scratch repro sources in `qdos/repro/` (moved to `evidence/`).
- The `build/` intermediates: only `bin/GWBASIC.AOU` and `bin/gwbasic.exe` are packaged.
- All scratch directories (`scratchpad/`), the Star Trek listings and their copies, and the kernel archive.

## Documents

- Rewritten: `docs/GWBASIC-KNOWN-LIMITATIONS.md` (the old file still listed SYSTEM, FILES and string
  operations as open).
- Updated: `docs/GWBASIC-KCTX-FIX2-HANDBACK.md` (Gates 2–4, kernel finding).
- New: `docs/GWBASIC-LANGUAGE-COVERAGE.md`, `docs/GWBASIC-RUNTIME-REPORT.md`, `README.md`,
  `HANDOVER.md`, `PATCH-NOTE.md`, `tests/historical/README.md`, `evidence/save_write_error/README.md`.
- `NOTES.md`: Stage 5 section brought up to date.

## Verification gates for this note

- Win32 `make test`: 42 of 42 on a clean build.
- QDOS full suite: 50 pass, 1 expected failure (`save_write_error`), 1 skip (`save_readonly`); see
  `docs/GWBASIC-RUNTIME-REPORT.md`.
- Fresh extraction: see `HANDOVER.md`, section "Verification".
