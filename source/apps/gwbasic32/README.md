# GW-BASIC for QDOS/386 (i386 GNU as 1.38 port)

A port of the Microsoft GW-BASIC 8086 interpreter source (MASM) to 32-bit i386 assembly, built with
GNU as 1.38 for QDOS/386, plus a Win32 build (MinGW, run under Wine) used as the reference for
regression tests. The port is a translation of the original assembly with a QDOS platform layer; it
is not a rewrite in C.

Read first: `HANDOVER.md` (what was delivered, status, the kernel finding), then
`docs/GWBASIC-KNOWN-LIMITATIONS.md`.

## Layout

| Path | Contents |
|---|---|
| `src/` | Translated 32-bit sources (`.S`, `.inc`). `src/deferred/lprint.S` is kept but not built. `src/CONVENTIONS.md` records the translation rules. |
| `qdos/` | QDOS platform layer (`platform_qdos.S`, `qdos_main.S`). |
| `Makefile` | Win32 build (MinGW i686, `make`; `make test`). |
| `Makefile.qdos` | QDOS build with GNU as 1.38 and i386-ld (a.out OMAGIC). |
| `tools/` | `intel2att.py` converts `src/` to AT&T syntax for GNU as 1.38; `verify_att.py` checks the conversion; `gen_*` and `unstub.py` are generators used during the port. |
| `tests/cases/` | Shared regression cases (`NAME.in` typed input, `NAME.exp` expected screen text). Run on Win32 and on QDOS. |
| `tests/qdos/` | QDOS harness (`run_qdos.py`), QDOS-only cases, and fixtures. |
| `tests/coverage/` | Statement and function probe and the matrix renderer. |
| `tests/historical/` | How to fetch and check the historical listings (Super Star Trek, Mandelbrot). |
| `docs/` | Known limitations, language coverage, runtime report, kernel hand-back. |
| `evidence/` | Files behind the kernel finding and the alignment control. |
| `logs/` | Test and run logs for this pass. |
| `bin/` | Built executables: `GWBASIC.AOU` (QDOS) and `gwbasic.exe` (Win32). |
| `SHA256SUMS` | Checksums of every file in the package. |

## Build and test

Win32 (needs MinGW i686 and Wine):

    make                       # build/gwbasic.exe, 64 KB workspace
    make test                  # tests/run_gw.py over tests/cases

QDOS (needs the R13B SDK and GNU as 1.38 / i386-ld in one directory):

    make -f Makefile.qdos R13B=/path/to/qdos386-R13B QDTOOLS=/path/to/bin
    make -f Makefile.qdos image R13B=... QDTOOLS=...   # build/qdos/launch-volume.img
    make -f Makefile.qdos run R13B=... QDTOOLS=...     # boot it in QEMU

QDOS tests run on a booted QEMU image. They need the KCTX-FIX2 kernel and its archive (not
included here; see `HANDOVER.md` for the path and hash used):

    QDOS_KERNEL=/path/to/kernels/qdos386-vga.img \
    QDOS_ARCHIVE=/path/to/qdos386-KCTX-FIX2-MAINLINE \
    QDOS_OUT=/tmp/qdos-tests \
    python3 tests/qdos/run_qdos.py -v --suite all

`--suite shared` runs only `tests/cases`, `--suite qdos` only `tests/qdos/cases`. A case may be
reported as `SKIP` with its reason (see `tests/qdos/run_qdos.py`); `SKIP` is never a pass.

## Status

Win32: 42 of 42 shared cases pass. QDOS: see `docs/GWBASIC-RUNTIME-REPORT.md` for the run used in
the hand-back. One case is an expected kernel finding (`save_write_error`), one is a harness skip
(`save_readonly`). Other limitations are in `docs/GWBASIC-KNOWN-LIMITATIONS.md`.
