# QDOS/386 Apollo 14 (R18) — graphics presentation and native-resolution EliteQL: handover

**Package**: `qdos386-APOLLO14-R18-GRAPHICS.tar.gz` (public-safe: no SuperBASIC, no Infocom, no Elite).

**Private add-on**: `qdos386-APOLLO14-ELITEQL-PRIVATE.tar.gz`. It contains the frozen `ELITEP.AOU` and the Elite
evidence pictures. Do not redistribute it.

**Source**: branch `r18-apollo14-graphics` on top of `a13-r17`.

| Tag | Content |
| --- | --- |
| `a14-gate1` | planar presenter |
| `a14-gate2` | session switching |
| `a14-gate3` | EliteQL |
| `a14-gate4` | compatibility and robustness |
| `a14-r18` | release |

## What R18 is

An **opt-in** kernel target, `make qdos386-r18-pc`, which builds `qdos386-r18-vga.img` and `qdos386-r18.img`. It is
R17 plus a **640×480×16 (VGA mode 12h) presentation path for INDEX8 surfaces**. Every change is inside
`#ifdef QDOS_R18`. These kernels rebuild byte-identical:

| Kernel | Image | SHA-256 prefix |
| --- | --- | --- |
| R17 | `qdos386-r17-vga.img` | `40968de4` |
| R17 | `qdos386-r17.img` | `f53bd88a` |
| R16B | `qdos386-r16b-vga.img` | `d073fbea` |
| R16A | `qdos386-smp-vga.img` | `b9ecb69a` |
| KCTX-FIX2 | `qdos386-vga.img` | `f8e5d5e1` |
| KCTX-FIX2 | `qdos386.img` | `7c675978` |

* **Views**: `SURF_OP_VIEW` b = 2 (`VGA12`, 1:1 centred) or 3 (`VGA12W`, each pixel twice as wide = the QL's own
  aspect). Rows are never scaled. A 256×256 surface is shown complete at (192, 112), or as 512×256 at (64, 112).
* **Colour**: colour = pixel & 15, through palette entries 0..15. The capability bit is `SURF_CAP_VGA12`. Earlier
  kernels refuse the new views with -4.
* **User override, no program change**: Task Manager **F3** cycles the selected job's presentation (program's choice →
  640×480 1:1 → 640×480 2× wide → 320×200 fit). `/BIN/VIEW con APP|FIT|VGA12|VGA12W` does the same from the shell. The
  kernel call underneath is `SURF_OP_SESS_VIEW`. This is how the frozen EliteQL is shown at full height.
* **Presenter cost**: the presenter does at most 64 rows per tick, because the SYSTEM poll holds the BKL with
  interrupts off. `SURF_OP_PRES_TIME` / `PLANAR TIME` measure it.
* **Public demo and test card**: `/BIN/PLANAR`. The host oracle `tests/a14/oracle.py` recomputes every screen pixel
  (layout, bitplanes, DAC).

Unchanged: SMP (8 CPUs, BKL, topology), the memory map, the file system, GCC, QLTerm, Hack, gameplay code, and the
320×200×256 and rendered-text-console paths.

## Build, boot, test

```sh
export QDOS386_TOOLS=/path/to/i386vm-tree         # GCC 1.40 / GAS 1.38 / i386-ld (see README.md)
make qdos386-r18-pc
sh scripts/a14-build-guest.sh                      # PLANAR VIEW + the R17 tools
sh scripts/a14-sysvol.sh vol.img                   # public-safe volume with the tools in /BIN
qemu-system-i386 -drive file=build/qdos386-r18/pc-vga/qdos386-r18-vga.img,format=raw,if=ide,index=0 \
                 -drive file=vol.img,format=raw,if=ide,index=1 -m 64 -smp 2
```

At the shell:

```
START PLANAR CARD 256 256 VGA12 30 /TMP/C.RAW /NEW    the test card, 1:1 in 640x480x16 (it shows itself; Alt+F1 back)
START PLANAR CARD 256 256 FIT 60 /TMP/D.RAW /NEW      a program that asks for the old 320x200 fit...
VIEW 4 VGA12W                                         ...shown 2x wide in 640x480 instead (TASKS /L: CON column)
```

With the private add-on: copy `ELITEP.AOU` to `/BIN`, then `START ELITEP /NEW`, then Ctrl+Esc, select it, **F3**, Esc.
Elite's whole 256×256 screen, dashboard included, is shown 1:1. Press F3 again for 512×256.

Tests (QEMU; one emulator at a time):

| Script | What it covers |
| --- | --- |
| `sh tests/a14/run-s1.sh OUT` | Stage 1: 9 test cards, every pixel checked |
| `sh tests/a14/run-g2.sh OUT` | Stage 2: session switching |
| `sh tests/a14/run-s4.sh OUT` | Stage 4: 1 and 8 CPUs, robustness |
| `sh tests/a14/run-elite.sh OUT ELITEP.AOU ["-smp 2"]` | Stage 3: needs the private fixture |
| `R12_QKERNEL=build/qdos386-r18/pc-vga/qdos386-r18-vga.img sh tests/r13a/run-gate.sh 2\|3 OUT` | R13A regressions |
| `sh scripts/a14-verify-package.sh EXTRACTED OUT` | fresh-package verification |

## Evidence

Results are in `docs/APOLLO14-RUNTIME-REPORT.md`. `evidence/` has the text summaries and the PLANAR screendumps.
Elite pictures are only in the private add-on.

## Docs

| File | Content |
| --- | --- |
| `docs/APOLLO14-AUDIT.md` | Stage 0 audit and contract |
| `docs/APOLLO14-DISPLAY-ABI.md` | modes, views, colour policy, override, ownership, compatibility |
| `docs/APOLLO14-PLANAR-VGA.md` | registers, plane writes, bounded work, host oracle |
| `docs/APOLLO14-RUNTIME-REPORT.md` | QEMU evidence |
| `docs/APOLLO14-KNOWN-LIMITATIONS.md` | known limitations |

The patch against R17 is `patches/r18-against-r17.patch`.

## Constraints kept

* No source changes to SMP, memory discovery, the BKL, file systems, GCC, QLTerm, Hack, GW-BASIC or gameplay.
* EliteQL: the frozen protected binary is used unchanged, and no Elite source is in the tree.
* Applications never touch VGA registers.
* The override changes presentation only, and its permission is checked.
* Frozen kernels are byte-identical.
* No VBE, SVGA, mouse, windows or sound.
