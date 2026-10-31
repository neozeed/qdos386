# qdos386-R15-AMP0 — master/worker (AMP) milestone, complete source

Source: branch `r15-amp-worker`, commit `7651795d7f2ea5d4dbf4bf8e3daac566fdd1b687`, from the frozen KCTX-FIX2 mainline (`5b6fc47`, tag
`kctx-fix2-mainline`, archive `778a3d61…`). The archive's own SHA-256 is given with the delivery; `SHA256SUMS` lists
every other file. Start with **`HANDOVER.md`**.

**Private archive**: like KCTX-FIX2 it carries SuperBASIC (`apps/superbasic/`) and the Infocom interpreter source
(`third_party/infocom/orig/`). Publish only a tree made by `scripts/qdos386-public-source.sh` and the public volume.

| Path | What |
| --- | --- |
| (root) | full source: kernel (AMP: `kernel/amp*.{c,h,s}`), boot loader, drivers, SDK (`sdk/include/qdos386amp.h`), C68/QL compatibility, Hack, QLTerm, tests (`tests/r15/`), docs (`docs/R15-*`), logs (`logs/r15/`) |
| `kernels/qdos386-amp-vga.img`, `qdos386-amp.img` | the opt-in AMP kernels (QEMU VGA / headless); `.map`; `ap-static-check.txt`; `qdos386-amp-vga.worker-disasm.txt` |
| `kernels/qdos386-vga.img`, `qdos386.img` | the ordinary single-processor kernels (`make`), identical to KCTX-FIX2 |
| `kernels/test-only/` | AMP test variants: `nosipi` (worker never started), `late` (worker READY seconds late), `burn` (worker computes continuously) — never use as the AMP kernel |
| `kernels/bios.bin`, `vgabios.bin` | SeaBIOS / VGA BIOS for the Tiny386 runs |
| `guest/` | `AMPT.aout` (test driver), `WORKER.aout` (demo), `SPINM.aout` |
| `volumes/r15-public.img` | public-safe system volume (no SuperBASIC, Infocom or story file) with AMPT, WORKER, SPINM in `/BIN` |
| `patches/` | `r15-against-kctx-fix2.patch`, `CHANGED-FILES.txt` |
| `emulator/` | Tiny386 R12C source (BSD-3-Clause) and `bin/tiny386_headless` |
| `toolchain/` | I386VM R1 toolchain source archive (GCC 1.40, GAS 1.38, binutils 1.94 ld) |

Build: `export QDOS386_TOOLS=<I386VM R1 tree>`; `make` (ordinary kernels); `make qdos386-amp-pc` (AMP kernels);
`sh scripts/r15-build-guest.sh` (AMPT, WORKER). Run: see `HANDOVER.md`. Tests: `tests/r15/run-gate1.sh` …
`run-gate3.sh`, `run-regress.sh`, `run-hack-smoke.sh`, `run-tiny.sh`, `check-ap-objects.py`, `oracle.py`.
Tested with QEMU 8.2.2 (`-machine pc -cpu pentium -smp 2 -m 32`, SeaBIOS) and Tiny386 R12C.

Licences as in KCTX-FIX2 (`PACKAGE.md` there; `docs/R14-LICENSE-PROVENANCE.md`): QLTerm GPL-3.0-or-later, optional
Minerva font GPL-2.0, Hack 1.0.3 BSD-style, Tiny386 BSD-3-Clause, toolchain GPL; SuperBASIC private; Infocom
interpreter unlicensed; no story file anywhere.
