# GW-BASIC i386 on QDOS/386 KCTX-FIX2 — Hand-back (Gates 0–4)

## 1. Baseline

- Kernel: `qdos386-KCTX-FIX2-MAINLINE.tar.gz`, branch `r14a-kctx-mainline-fix`, source `b1db28a`, verification
  `5b6fc47`, tag `kctx-fix2-mainline`.
- Kernel image used: `kernels/qdos386-vga.img`, SHA256 `f8e5d5e1f7e5c698ed13cc57159f7c9a7faeffb8fcf523e185625a57aec0d4c3`
  (verified again for every run in this pass).
- The kernel was **not modified**. Option A, Option B, and any port-side alignment workaround were not applied.
  No FOR entry size or push/pop width was changed to hide a kernel defect.
- Interpreter for the final runs: `build/qdos/GWBASIC.AOU`, SHA256 `c7dd8abc585f814016f681f8b4aba2dcad91e6c828724b35a72a724e3d2d8e40`
  (in `bin/`). Win32 reference build: `build/gwbasic.exe`, SHA256 `de67f39b92c59c548b7444e7f9ad2c1242132cb1da2ce420b4b55e809515aa83`.

## 2. Gate 0 (kernel adoption) and Gate 1 (SAVE/LOAD)

### Gate 0: PASS

Unchanged `GWBASIC.AOU` on the unchanged launch volume, normal kernel. Flat FOR/NEXT 16000, 16400, 20000, 200000;
nested 100 × 1000; GOSUB 20000: each `DONE` followed by `Ok`. No #DF, no kernel halt, a GW-BASIC process fault ended
only that process, LEAK before = after, volume clean after the run. The MID$/LEFT$/RIGHT$ fault and the SAVE/LOAD
failures seen then were interpreter defects, fixed below.

### Gate 1: PASS for the listed items; one kernel-limited item and one skip

| Test | Result |
|---|---|
| `10 PRINT "HELLO"` → `SAVE "HELLO.BAS"` → `NEW` → `LOAD` → `LIST` → `RUN` | PASS |
| Persistence after a fresh boot from the saved volume | PASS (SHA256 identical after save and after boot) |
| Overwrite an existing file (`SAVE "A"`, truncates) | PASS (`save_load`) |
| Missing file (`LOAD "NOSUCH"`) | PASS: File not found |
| Empty name (`SAVE ""`, `LOAD ""`) | PASS: Bad file name |
| Malformed or short files (`load_errors`, `load_errors_83`) | PASS: Bad file mode, program unchanged |
| ASCII SAVE and MERGE (`save_ascii`, `save_ascii_big`, `merge_ascii`, `merge_errors`) | PASS |
| Directory full, disk full empty and partial (`save_dirfull`, `save_diskfull_*`) | PASS; no partial file left |
| 12-character name `SAVE` | FAIL vs GW-BASIC: Device I/O Error. QDOS legacy names are 8.3 flat in the root (kernel limit). Open. |
| Read-only media (`save_readonly`) | SKIP: QEMU 8.2.2 cannot attach a read-only IDE disk. Not tested. |
| Block write EIO during SAVE (`save_write_error`) | FAIL: `Disk write protected` instead of `Device I/O Error`. Kernel finding, section 4. |

Saved format: the program image as held in memory, with 4-byte absolute link fields and an end link. This is a native
image, not the DOS tokenized format. Compatibility with DOS `.BAS` files is not claimed.

## 3. Interpreter defects fixed, and Gates 2–3

### Defects fixed in this pass

1. **String slicing stack fault** (`src/gweval.S`, `ISFUN`): the second argument was pushed as 4 bytes but popped
   as 2 (`PREAM2`). Fix: `push si`. Verified by `string_slice` on both builds.
2. **SAVE file name** (`src/bifile.S`): the name pointer was lost across `CHRGTR`/`FILEND`. Fix: keep it in
   `SAVNAME`, as LOAD does. Verified by `save_load` and the fresh-boot check.
3. **`PRINT A#` and `PRINT 1E3#`** (`src/stage3stubs.S` FILGET; `src/math1.S` FIN): an off-by-one before the `#`
   test, and a `#` after an exponent was not consumed. Verified by `dp_var_print`.
4. **LLIST infinite error loop** (`src/stage3stubs.S` FINPRT): the console was not restored after LLIST, so the
   error message went to the printer, whose error started again. Verified by `llist_unsupported`.
5. **READ always Out of DATA** (`src/gwmain_b2.S`): the start-of-line sentinel and the line-link size were wrong
   for the QDOS line layout. Verified by `read_data`.
6. **INT for singles** (`src/math2.S`, `SNGINT`): also rewritten without `BSR`, `PUSHAD` or `POPAD`, which GNU as
   1.38 does not assemble. Verified by `int_edge`.

### Gate 2: strings, arrays, garbage collection, numerics: PASS on QDOS

`string_concat`, `string_funcs`, `string_slice`, `string_gc`, `string_array_gc`, `print_string`, `dim_array`,
`dim_more` (2-D numeric and string arrays), `int_*`, `dp_ops`, `dp_var_print`, `arith_mixed`, `def_fn`: all PASS on
QDOS and Win32. Known numeric limit: DP transcendental functions return single precision and `EXP(1)` prints
`2.718267` (see `docs/GWBASIC-KNOWN-LIMITATIONS.md`, section 5).

### Gate 3: SYSTEM, FILES, coverage, historical programs, WHILE/WEND: PASS with the limits below

- `SYSTEM` (`system_exit`): PASS. Returns to the QDOS shell prompt.
- `FILES` (`files_pattern`): PASS.
- Statement and function coverage: `docs/GWBASIC-LANGUAGE-COVERAGE.md` (136 items, probed on the Win32 build). Not
  implemented on purpose: file I/O with `#`, printer, graphics, machine access (stubs, see the limitations).
- Historical programs: Mandelbrot (MIT) runs on both builds. Super Star Trek runs on both builds with ten spaced
  lines; the tokenizer divergence behind the spacing is open (limitations, section 4). Procedure and evidence:
  `tests/historical/README.md`, `docs/GWBASIC-RUNTIME-REPORT.md`.
- FNDWND WHILE/WEND: the `whl_*` cases (WEND with FOR entries on the stack, nested skips, both mismatch errors)
  PASS on both builds. The FNDWND skip constant (`FORSZC-1`, which should be `FE_SIZE-2`) is still a latent
  mismatch noted in `NOTES.md`; no tested input reaches it, and it was not changed.

## 4. Kernel finding: block write error reaches SAVE as write-protected (for the OS builder)

**Status: reproduced. Not a port defect. The port was not changed to hide it, and the expectation was not changed.**

What fails: `tests/qdos/cases/save_write_error`. A `SAVE` runs while QEMU's blkdebug layer fails the first block write
once with EIO (`write_aio`, `errno = 5`, `once = on`; `evidence/save_write_error/blkdebug.conf`).

- Expected: `Device I/O Error`.
- Observed on the KCTX-FIX2 kernel: `Disk write protected` (the port maps the kernel's −80 to code 19).

Minimal reproduction with no GW-BASIC code (`evidence/save_write_error/errprobe.c`, a C program calling
`fopen("IOE.DAT","w")`):

| Setup | Result |
|---|---|
| No fault injected | `FOPEN OK` |
| blkdebug write fault as above | `FOPEN FAILED errno=30` (EROFS) |

So the kernel's file open reports EROFS where the block layer returned EIO. This is reproduced without GW-BASIC.

Second observation, with a different injection: the archive's own `WRTTEST ioerr` (`QDOS_BLK_FAULT=io:N`, applied in
the block driver) reports `create failed with EIO` (errno 5). The two injection methods differ, so this does not yet
decide whether the kernel maps EIO to EROFS on one path only. Suggested check: reproduce both with one method.

What the OS builder can do with this: the EIO → EROFS mapping for block write errors on the create/open path
(`fopen` in libqdos, or the kernel file layer it calls). The port's mapping (`platform_qdos.S` `plat_wrerr`) is
documented in `evidence/save_write_error/README.md`. No change is requested on the port side.

Evidence: `evidence/save_write_error/` (README, `errprobe.c`, `blkdebug.conf`, QEMU sequences and screen captures,
`SHA256SUMS.kernel-inputs`). Logs: `logs/qdos-controls-and-multiapp.txt`, `logs/qdos-full-suite.log`.

## 5. Gate 4: regression set, multi-app session, package

- Win32 `make test`: 42 of 42 (`logs/win32-make-test.log`).
- QDOS full suite (`tests/qdos/run_qdos.py --suite all`): 50 pass, 1 expected fail (`save_write_error`), 1 skip
  (`save_readonly`), 52 cases (`logs/qdos-full-suite.log`).
- Long-loop stress on KCTX-FIX2: PASS (`long_loops`, `logs/qdos-long-loops.log`). The earlier double fault with
  misaligned resume is not seen on this kernel.
- Bounded multi-app session: GW-BASIC runs a program, `SYSTEM` returns to the shell, the shell starts `HELLO.AOU`,
  and it prints its line. PASS (`logs/qdos-controls-and-multiapp.txt`).
- Runtime report: `docs/GWBASIC-RUNTIME-REPORT.md`.
- Package, checksums and fresh-extraction verification: `HANDOVER.md`.

## 6. Still open (not claimed)

- Kernel: the EIO → EROFS mapping (section 4). Expected failure `save_write_error` until fixed.
- Files: 12-character names on QDOS (kernel legacy names); read-only media; short read and write; file-close failure.
- Tokenizer: keywords directly followed by a letter or digit (`FOR I=1TO3`) give Syntax error here. Original
  behaviour not verified. Keyword gap (`CALL`, `CHAIN`, `CHDIR`, `MKDIR`, `RMDIR`, `ENVIRON`, `ERDEV`, `VIEW`,
  `WINDOW`, `PMAP`, `PALETTE`, `SHELL`, `GET`, `PUT`): token numbers collide with `STMDSP` slots 149–158.
- Numerics: DP transcendental precision; `EXP(1)` accuracy.
- Time and device stubs (`TIME$`, `DATE$`, `TIMER`, `STICK`, `ERR`, `ERL`), graphics and sound stubs, port I/O stubs.
- FNDWND skip constant (latent).
- Not compared with the original GW-BASIC: no reference binary or listing output was available in this environment.

## 7. Reproduction

Boot: the normal kernel `kernels/qdos386-vga.img` and the launch volume with the built `GWBASIC.AOU`. The QDOS suite:

    QDOS_KERNEL=/path/to/kernels/qdos386-vga.img QDOS_ARCHIVE=/path/to/qdos386-KCTX-FIX2-MAINLINE \
    QDOS_OUT=/tmp/qdos-tests python3 tests/qdos/run_qdos.py -v --suite all

Controls for section 4: `evidence/save_write_error/README.md`. Multi-app session: `logs/qdos-controls-and-multiapp.txt`.
