# qdos386-R11H-FIX1

Base: R11H source `a15bd80` (frozen). Branch `r11h-fix1`. Final source commit: `4affc37`
(the commit that holds this package's docs; later commits, if any, are listed in the
release notes that accompany the tarball).

Contents
- Source tree (git archive of the final source commit): kernel, platform, SDK, tools, scripts,
  docs, tests, patches, third_party.
- build/r11h/: MULTI, MULTIA, MULTIB, COLLIDE, NEG (release set) and FAULT (test only).
- build/r11h-neg/: malformed a.out inputs (not runnable); the same ten files are inside win1-r11h-neg.img.
- build/r11h-maps/: link maps for the R9 and R8 memory maps.
- build/fat16/win1-r11h.img: release volume (no story), sha256 672806ad2c2ca337c453...
- build/fat16/win1-r11h-neg.img: negative test volume (release set plus the ten malformed inputs).
- build/fat16/win1-r11h-fault.img: test volume (release set plus FAULT.AOU).
- build/pc-r11h/, build/pc-r11h-vga/: PC shell and VGA kernel images (release).
- build/pc-r11h-none/, build/pc-r11h-pin/, build/i386vm-r11h-pin/: clock-mode variants for the regression.
- build/pc-fault/: TEST ONLY. Kernel built with -DQDOS_TEST_FAULT_LAUNCH=4; never a release image.
- build/i386vm-r11h-rel/, build/i386vm-r11h-neg/: i386VM images (release and negative).
- build/pc-r11f-none-run/bios.bin, vgabios.bin: firmware used by the Tiny386 runs.
- logs/fix1/: matrix, regression, QEMU VGA capture, fault and Phoon 2106 transcripts.

Not included (deliberately)
- Emulator binaries (Tiny386, i386VM harness). They are identified by hash in docs/R11H-FIX1-RUNTIME-REPORT.md
  and built from the emulator source and patches.
- The private Zork I story volume and any derived smoke volume.
- The historical GCC 1.40 / GAS / i386-ld tree (identified by hash in docs/R11H-FIX1-BASELINE.md).

Toolchain
- Building needs the historical tree: QDOS386_TOOLS and I386VM both point to it.
- Canonical commands: docs/R11H-FIX1-BUILD-REPRO.md.

Verification
- docs/R11H-FIX1-RUNTIME-REPORT.md: results, labelled as scripted, with limits.
- SHA256SUMS lists every file in this package except itself.
