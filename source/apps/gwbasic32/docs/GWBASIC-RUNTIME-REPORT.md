# GW-BASIC on QDOS/386 — Runtime Report (Gates 2–4)

## Environment

| Item | Value |
|---|---|
| QDOS kernel | KCTX-FIX2 normal kernel, `kernels/qdos386-vga.img`, SHA256 `f8e5d5e1f7e5c698ed13cc57159f7c9a7faeffb8fcf523e185625a57aec0d4c3`. Unmodified. Archive `qdos386-KCTX-FIX2-MAINLINE.tar.gz` (branch `r14a-kctx-mainline-fix`, source `b1db28a`, verification `5b6fc47`, tag `kctx-fix2-mainline`). |
| Interpreter (QDOS) | `build/qdos/GWBASIC.AOU` (copied to `bin/`), SHA256 `c7dd8abc585f814016f681f8b4aba2dcad91e6c828724b35a72a724e3d2d8e40`. Built with GNU as 1.38 and i386-ld, a.out OMAGIC 0407. |
| Launch volume | `build/qdos/launch-volume.img` (a copy of the R13B system volume, `e26b0ed4…`, with `GWBASIC.AOU` added; the copied `GWBASIC.AOU` is `c7dd8abc…`). The whole-image hash is not reproducible: two identical rebuilds gave different hashes (FAT timestamps), so the image is not a stable identity. An earlier build recorded `1871dc15…`; it cannot be matched here. |
| Interpreter (Win32) | `build/gwbasic.exe` (copied to `bin/`), SHA256 `de67f39b92c59c548b7444e7f9ad2c1242132cb1da2ce420b4b55e809515aa83`. MinGW i686, run under Wine 9.0. |
| Emulator | QEMU 8.2.2, `-machine pc -cpu pentium -m 16`, SeaBIOS. |
| Host | Linux 6.18.44. |

## Results

| Suite | Cases | Pass | Fail | Skip | Notes |
|---|---|---|---|---|---|
| Win32, `make test` (`tests/run_gw.py`) | 42 | 42 | 0 | 0 | `logs/win32-make-test.log` |
| QDOS, full (`tests/qdos/run_qdos.py --suite all`) | 52 | 50 | 1 | 1 | `logs/qdos-full-suite.log`. The failure is the expected kernel finding `save_write_error`. The skip is `save_readonly`. |

The QDOS run has 52 cases: 51 executed (50 pass, 1 fail) and 1 skipped. The harness's summary line
"51 case(s)" counts the executed cases, not the skip.

### QDOS failures and skips, in detail

- **`save_write_error` — FAIL (expected, kernel).** `SAVE` with one injected block-write EIO prints
  `Disk write protected` where the expectation is `Device I/O Error`. Evidence and the hand-off:
  `evidence/save_write_error/README.md`, `docs/GWBASIC-KCTX-FIX2-HANDBACK.md` section 4. The expectation was
  not changed.
- **`save_readonly` — SKIP (harness).** QEMU 8.2.2 cannot attach a read-only IDE hard disk in this setup
  (`Block node is read-only` at `ide-hd` realize). The read-only mapping in `platform_qdos.S`
  (`plat_wrerr`) is not exercised. The skip reason is printed by the harness.

### Earlier QDOS run (history)

`logs/qdos-suite-r18-earlier-run.log` (50 cases on the same build, before the final changes): 48 pass,
2 fail, 1 skip. The two failures were `save_write_error` (expected) and `load_errors`. `load_errors`
used Win32-only fixture names (`BADSTRUCT.BAS`), which cannot exist on the flat 8.3 QDOS volume. It now has
a QDOS replacement with the same expectations and the 8.3 name `BADSTR.BAS` (`tests/qdos/cases/load_errors`),
which passes (`logs/qdos-load-errors-replacement.log`).

## What was exercised on QDOS

All QDOS results are from the full suite above unless stated.

- **Program control and loops.** `for_loop`, `for_gosub`, `nested_for`, `gosub_return`, `return_nogosub`,
  `next_nofor`, `whl_for_skip`, `whl_skip_nested`, `whl_wend_nowhile`, `whl_while_nowend`.
  `long_loops` (run separately as well, `logs/qdos-long-loops.log`): FOR to 16000, 16400, 20000 and 200000,
  nested 100 × 1000, and 20000 GOSUB calls, each followed by its output and Ok. This is the stress set that
  previously double-faulted with the misaligned resume (see `NOTES.md`, Stage 5). It passes on KCTX-FIX2.
- **Strings.** `string_concat`, `string_funcs` (LEFT$, RIGHT$, MID$, LEN, STR$, VAL, INSTR, …), `string_slice`,
  `string_gc`, `string_array_gc`, `print_string`. These cover Gate 2's list: concatenation, slicing,
  arrays of strings and repeated garbage collection.
- **Numerics.** `arith_mixed`, `int_ops`, `int_single`, `int_edge`, `dp_ops`, `dp_var_print`, `print_arith`,
  `div_zero`, `div_zero_run`, `def_fn`, `vars_num_str`, `dim_array`, `dim_more`. The `dp_var_print` case covers
  `PRINT 1E30#` → ` 1D+30 `. The transcendental precision limit is in `docs/GWBASIC-KNOWN-LIMITATIONS.md`, section 5.
- **Data.** `read_data` (READ, DATA, RESTORE, Out of DATA).
- **Files (Gate 1, extended in Gate 2–3).** `save_load` (SAVE, NEW, LOAD, LIST, RUN; overwrite),
  `save_ascii`, `save_ascii_big`, `save_dirfull`, `save_diskfull_empty`, `save_diskfull_partial`,
  `load_errors`, `load_errors_83`, `merge_ascii`, `merge_errors`, `files_pattern` (`FILES`), `system_exit`
  (`SYSTEM` returns to the QDOS shell prompt), `llist_unsupported` (LLIST/LPRINT: Advanced Feature, no loop).
- **Statement and function coverage.** `docs/GWBASIC-LANGUAGE-COVERAGE.md` (the probe, 136 items, run on the Win32
  build; the QDOS build runs the same interpreter code).

## Additional QDOS sessions

- **Multi-app session (Gate 4).** `logs/qdos-controls-and-multiapp.txt`. GW-BASIC is started, a program runs,
  `SYSTEM` returns to the shell, the shell starts `HELLO.AOU` (a separate guest-compiled C program from the R11L
  oracle set), and that program prints its line. The application exits with status 7; the status was not captured.
- **save_write_error controls.** `logs/qdos-controls-and-multiapp.txt`. A C program calling `fopen` gets `FOPEN OK`
  with no fault and `FOPEN FAILED errno=30` with the blkdebug write fault. The GW-BASIC result follows from the
  same kernel path.

## Historical programs (Gate 3)

Not part of the automated suite. Procedure and inputs: `tests/historical/README.md`.

- **Mandelbrot, text mode (MIT, Eric Furst).** Win32 build: the full 75 × 49 set and its two summary lines
  (`logs/historical/mandelbrot-win32-transcript.txt`). QDOS: the two summary lines appear in 60
  screen captures taken two seconds apart after the last input (`logs/historical/hist_mbrot-qdos-summary.txt`).
  Interpreter error text in the captures: none. Intermediate rows scrolled off before the captures, so the QDOS
  run checks the final lines only.
- **Super Star Trek (spaced copy, see `tests/historical/README.md`).** Win32: the scripted session runs with no
  error text (`logs/historical/startrek-win32-summary.txt`). QDOS: `LONG RANGE SCAN`, `STARDATE 2200.2` and the
  mission-end text appear in the captures, with no interpreter error text (`logs/historical/hist_startrek-qdos-summary.txt`).
  The only change to the listing is the ten spaced lines described in `tests/historical/README.md`; the original is
  not redistributed.

## What is not claimed

- No output was compared with the original Microsoft GW-BASIC, because it was not available here.
- The QDOS historical runs are smoke checks through screen captures. They are not byte-exact transcripts.
- Read-only media, the QDOS disk-full path beyond the two cases above, and the kernel-side EIO mapping are open
  (see the handback).
- The statement coverage matrix records observed output; "runs" is not a claim of fidelity.
