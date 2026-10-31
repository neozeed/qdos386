# qdos386-KCTX-FIX2-MAINLINE — complete source release of the corrected kernel line

Source: branch `r14a-kctx-mainline-fix`, commit `@COMMIT@`, based on frozen R14A `7be7353` (R14 kernel base
`43a1f448`) with the KCTX-FIX1 correction (`46a06e6`) promoted into the normal kernel. The archive's own SHA-256 is
given with the delivery; `SHA256SUMS` lists every other file. Start with **`HANDOVER.md`**.

**Private archive.** It carries SuperBASIC (`apps/superbasic/`) and the Infocom interpreter source
(`third_party/infocom/orig/`): see Licences. Publish only a tree made by `scripts/qdos386-public-source.sh` and the
public volume.

## Contents

| Path | What |
| --- | --- |
| (root) | full source tree: kernel, boot loaders (`platform/pc`), drivers, SDK (`sdk/`: libc, libc68, libm, libgcc-qd, libqdos, headers, examples), C68/QL compatibility (`sdk/qlcompat`), apps and shells, Hack 1.0.3 (`third_party/hack103-ql`), QLTerm (`third_party/qlterm-c-r1`), image tools (`tools/`, `scripts/`), tests (`tests/`), docs, logs |
| `kernels/qdos386-vga.img`, `qdos386.img` | the current kernels (QEMU VGA / headless for Tiny386), from `make`; `.map` symbol maps; `resume-convention.txt` |
| `kernels/negative-control/` | the original defective R14 kernels from the R14 delivery, read-only (0444) |
| `kernels/bios.bin`, `vgabios.bin` | the SeaBIOS / VGA BIOS used by the Tiny386 runs |
| `patches/` | `kctx-fix2-against-r14a.patch` (minimal review patch), `CHANGED-FILES.txt` |
| `probe/` | ALIGNT (ESP residues 0-3 × timer/trap/yield) and the supplied `spin_aligned` / `spin_misaligned` |
| `volumes/qdos386-public.img` | public-safe system volume (no SuperBASIC, no Infocom interpreter, no story file) |
| `emulator/` | Tiny386 R12C: full source (`src/`, BSD-3-Clause `LICENSE`) and `bin/tiny386_headless` |
| `toolchain/` | the I386VM R1 toolchain source archive (GCC 1.40, GAS 1.38, binutils 1.94 ld; GPL), as supplied |

## Build

    export QDOS386_TOOLS=<I386VM R1 tree>        # bin/i386-gcc, bin/as, bin/i386-ld (toolchain/ archive, its README)
    make                                        # = make qdos386-pc: build/qdos386/pc-vga/qdos386-vga.img, build/qdos386/pc/qdos386.img
    sh tests/kctx/build.sh build/kctx           # the probes
    sh scripts/qdos386-public-vol.sh OUT.img    # the public-safe system volume (needs the userland built: scripts/r14a-sysvol.sh)
    sh tests/r14/check-frozen.sh                # frozen R12..R13B images; tests/r14a/check-r14-identical.sh R14 identity
    make qdos386-check-pc                       # test-only kernel that checks every resume hand-off

Tested with: QEMU 8.2.2 (`-machine pc -cpu pentium -m 16`), Tiny386 R12C (`emulator/`), toolchain binaries
`i386-gcc 3567f3be…`, `cc1 1dd52eeb…`, `as 47369dd5…`, `i386-ld dfdb3965…` (`logs/kctx2/stage0/inputs.txt`).

## Evidence

`docs/KCTX-FIX2-AUDIT.md`, `docs/KCTX-FIX2-IMPLEMENTATION.md`, `docs/KCTX-FIX2-RUNTIME-REPORT.md`,
`docs/KCTX-FIX2-KNOWN-LIMITATIONS.md`; logs `logs/kctx2/` (QEMU and Tiny386 transcripts, screenshots, judges) and the
KCTX-FIX1 reference (`docs/KCTX-FIX1-*`, `logs/kctx/`, including the gdb first-divergence trace).

## Licences

QDOS/386 kernel, SDK and tools: the project's own terms. QLTerm C R1: GPL-3.0-or-later (`third_party/qlterm-c-r1`,
`docs/R14-LICENSE-PROVENANCE.md`). Minerva 1.98 QL font: GPL-2.0, optional (`third_party/minerva`, not in any kernel).
Hack 1.0.3: © Stichting Centrum voor Wiskunde en Informatica / Jay Fenlason (BSD-style), QL port files © neozeed
(BSD-3-Clause) (`third_party/hack103-ql/orig`). Tiny386: BSD-3-Clause. Toolchain: GPL (GCC, GAS, binutils).
**SuperBASIC** (`apps/superbasic/`): private research, not freely redistributable. **Task Force '87 interpreter**
(`third_party/infocom/orig/`): no licence granted; no story file is included anywhere.
