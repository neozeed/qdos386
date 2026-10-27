# qdos386-R11H

Source commit: a15bd80 (branch r11h). Phase 2 checkpoint: a3c432a.

Contents
- Source tree (git archive of a15bd80): kernel, platform, SDK (sdk/), tools, scripts/, docs/, tests/.
- build/r11h/: MULTI, MULTIA, MULTIB, COLLIDE, NEG (a.out, slot 1/2/3 as linked).
- build/r11h-neg/: malformed a.out test inputs (not runnable programs).
- build/r11h-maps/: link-map builds for the R9 and R8 memory maps.
- build/fat16/win1-r11h.img: release volume (no story). sha256 672806ad...477b
- build/fat16/win1-r11h-neg.img: negative test volume (release volume plus malformed inputs).
- build/pc-r11h/, build/pc-r11h-vga/: PC kernel images (shell, VGA console).
- build/i386vm-r11h-rel/, build/i386vm-r11h-neg/: i386VM binaries.
- logs/: matrix, regression, pinned Phoon and NEG runs, QEMU VGA evidence.

Not included (deliberately)
- The private Zork I volume (win1-r11h-zork.img) and the QEMU smoke volume (win1-r11h-smoke.img),
  which carries the same story. The story is for internal testing only.
- Zork transcripts from the matrix logs.

Toolchain
- Building the programs needs the historical GCC 1.40 / GAS / i386-ld tree (QDOS386_TOOLS).
- Building the C library needs sdk/libc, sdk/libm, sdk/compiler, sdk/lib (see docs/R11H-RUNTIME-REPORT.md section 7).

Reproducibility
- The five R11H programs rebuild byte-identical from a clean checkout of a15bd80.
- The PC kernel and i386VM images do NOT reproduce from the documented make targets (see the runtime report).

Verification
- SHA256SUMS lists every file in this package except itself.
