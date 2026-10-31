# Handover: GW-BASIC on QDOS/386 (GNU as 1.38), Gates 0–4

## What this is

A port of the Microsoft GW-BASIC 8086 interpreter (MASM source) to 32-bit i386 assembly. It is built with
GNU as 1.38 for QDOS/386 (`Makefile.qdos`). A Win32 build (`Makefile`, MinGW under Wine) is the reference
for regression tests. The port converts the original Intel sources with `tools/intel2att.py`; it is not a
rewrite in C.

The work was run on the normal KCTX-FIX2 kernel, unmodified. Nothing in this package changes the kernel.

## Status

| Gate | Result | Where |
|---|---|---|
| 0: kernel adoption | PASS | `docs/GWBASIC-KCTX-FIX2-HANDBACK.md` §2 |
| 1: SAVE/LOAD | PASS for the listed items. 12-character names (kernel limit) and read-only media not passed or not tested. | handback §2; limitations §2 |
| 2: strings, arrays, GC, numerics | PASS on QDOS and Win32 | handback §3 |
| 3: SYSTEM, FILES, coverage matrix, historical programs, WHILE/WEND | PASS with the limits in the limitations doc | handback §3; `docs/GWBASIC-LANGUAGE-COVERAGE.md`; `tests/historical/` |
| 4: regression set, multi-app session, package | Win32 42/42; QDOS 50 pass, 1 expected kernel failure, 1 skip; multi-app PASS; package verified below | `docs/GWBASIC-RUNTIME-REPORT.md`; `logs/` |

**One expected failure is a kernel finding.** `save_write_error`: a block write EIO during SAVE reaches the
program as `Disk write protected` (kernel errno 30, EROFS). A minimal C program shows the same mapping with
no GW-BASIC code involved (`FOPEN OK` without the fault, `FOPEN FAILED errno=30` with it). The port was not
changed to hide this, and the expectation was not changed. Details and evidence:
`evidence/save_write_error/README.md`, `docs/GWBASIC-KCTX-FIX2-HANDBACK.md` §4. The OS builder should look at it.

## Deliverables

| Item | Path | SHA256 |
|---|---|---|
| QDOS interpreter | `bin/GWBASIC.AOU` | `c7dd8abc585f814016f681f8b4aba2dcad91e6c828724b35a72a724e3d2d8e40` |
| Win32 interpreter | `bin/gwbasic.exe` | `de67f39b92c59c548b7444e7f9ad2c1242132cb1da2ce420b4b55e809515aa83` |
| Source | `src/`, `qdos/platform_qdos.S`, `qdos/qdos_main.S`, `tools/` | see `SHA256SUMS` |
| Build | `Makefile`, `Makefile.qdos` | see `SHA256SUMS` |
| Tests | `tests/` (Win32 cases, QDOS harness and cases, probe, historical procedure) | see `SHA256SUMS` |
| Docs | `docs/`, `NOTES.md`, `PATCH-NOTE.md`, `README.md` | see `SHA256SUMS` |
| Evidence and logs | `evidence/`, `logs/` | see `SHA256SUMS` |
| Checksums | `SHA256SUMS` (every file except itself) | |

The package archive is `gwbasic32-qdos386-gas138.tar.gz`, with a checksum file next to it.

## Kernel and environment (not included)

- Kernel archive: `qdos386-KCTX-FIX2-MAINLINE.tar.gz` (branch `r14a-kctx-mainline-fix`, source `b1db28a`,
  verification `5b6fc47`, tag `kctx-fix2-mainline`). Kernel image used: `kernels/qdos386-vga.img`, SHA256
  `f8e5d5e1f7e5c698ed13cc57159f7c9a7faeffb8fcf523e185625a57aec0d4c3`. The archive is kept separate; it is
  not in this package. Its harness scripts (`scripts/r12-run-qemu.sh`) are needed for the QDOS tests.
- QDOS build: the R13B SDK (`qdos386-R13B`) and GNU as 1.38 / i386-ld 1.94 in one directory
  (`QDTOOLS`). The build used the R13B-SHELL-USABILITY SDK tree and the R11L host tools.
- Host tools: Wine 9.0 (Win32 tests), QEMU 8.2.2 (QDOS tests), mtools (`mcopy`, `mdir`), Python 3, MinGW i686
  binutils (Win32 build).

## Build and test

See `README.md`. In short:

    make && make test                                       # Win32
    make -f Makefile.qdos R13B=... QDTOOLS=...              # QDOS image (build/qdos/GWBASIC.AOU)
    QDOS_KERNEL=... QDOS_ARCHIVE=... python3 tests/qdos/run_qdos.py -v --suite all

The QDOS suite takes about an hour on the test host (each case boots QEMU; some cases wait 30 seconds).

## Verification

Results of the checks on this tree are in `logs/` and `docs/GWBASIC-RUNTIME-REPORT.md`.

Fresh-extraction check (the archive unpacked into an empty directory, then built and tested there):

- `sha256sum -c` on the archive's sidecar file: OK. `SHA256SUMS` in the extracted tree: 228 of 228 OK.
- Win32: `make` builds `build/gwbasic.exe` (`de67f39b…`); `make test`: 42 of 42.
- QDOS: `make -f Makefile.qdos image` (launch volume) and `make -f Makefile.qdos` (GWBASIC.AOU): the rebuilt
  `build/qdos/GWBASIC.AOU` is byte-identical to `bin/GWBASIC.AOU` (`c7dd8abc…`).
- QDOS suite (`tests/qdos/run_qdos.py --suite all`, KCTX-FIX2 kernel `f8e5d5e1…`): 51 cases executed,
  50 pass, 1 fail (`save_write_error`, the expected kernel finding), 1 skip (`save_readonly`).

The launch volume's whole-image hash is not reproducible across rebuilds (FAT timestamps differ between two
identical builds), so the volume is identified by its contents: the `GWBASIC.AOU` inside it is `c7dd8abc…`
and the base system volume is `e26b0ed4…`.

## Not in this package, and why

- **Star Trek listing** (`superstartrek.bas`): no clear redistribution licence. `tests/historical/fetch_listings.sh`
  fetches it from the recorded commit and checks its SHA256. `PROVENANCE.md` records the decision, which is the
  user's to change.
- **Kernel archive and SDK**: separate packages, referenced by path and hash above.
- **Build intermediates, scratch files, the stray root `a.out`**: removed.

## Open items

Listed with reasons in `docs/GWBASIC-KNOWN-LIMITATIONS.md` and the hand-back §6. The main ones:

1. Kernel: EIO → EROFS on block write errors through `fopen` (`save_write_error`).
2. QDOS names are 8.3 in the root only; 12-character names give Device I/O Error.
3. Tokenizer: keywords directly followed by a letter or digit (`FOR I=1TO3`) are a Syntax error here. The original
   behaviour was not verified.
4. Keyword gap for `CALL`, `CHAIN`, `CHDIR`, `MKDIR`, `RMDIR`, `ENVIRON`, `ERDEV`, `VIEW`, `WINDOW`, `PMAP`,
   `PALETTE`, `SHELL`, `GET`, `PUT` (token numbers collide with `STMDSP` slots 149–158).
5. DP transcendental functions return single precision; `EXP(1)` prints `2.718267`.
6. Stubs: file I/O with `#`, printer, graphics, sound (silent), port I/O, `TIME$`/`DATE$`/`TIMER` fixed values.
7. No comparison with the original Microsoft GW-BASIC was possible (no reference available here).
