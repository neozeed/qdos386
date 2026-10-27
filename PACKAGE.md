# qdos386-R11I-BLOCK

Base: frozen R11H-FIX1 `4affc37` (archive SHA256 `43fe3a125355991df151057715a510d736cbf2349f921ad1fdef2971d6dcbf22`),
unchanged.  Branch `r11i-block`.  Final source commit: `bc9a90f`.

Contents
- Source tree (git archive of `bc9a90f`): kernel, platform (both i386VM disk backends), SDK, tools,
  scripts, docs, tests, patches, third_party, logs.
- patches/i386vm-block.patch + harness/i386vm-block/blockdev.{c,h}: the separate, optional harness
  change.  Build instructions: README.md ("R11I - Build") or scripts/r11i-build-harness.sh.
- build/r11i/block/qdos386-r11i-block.bin: kernel-only i386VM image for the block device (release).
- build/r11i/block-pin/: pinned-clock block variant used by the Phoon regression (test variant).
- build/r11i/spare-test/: TEST ONLY.  Block kernel with -DQDOS_TEST_SPARE; never a release image.
- build/fat16/win1-r11h.img: example read-only FAT16 volume (project programs and texts, no story).
- build/fat16/win1-r11h-neg.img: negative-test volume (release set plus ten malformed inputs).
- build/i386vm-r11h-rel/, build/i386vm-r11h-neg/: legacy RAM-disk i386VM images (= R11H-FIX1).
- build/pc-r11h/, build/pc-r11h-vga/ and the other R11H-FIX1 build outputs: unchanged, byte-identical.
- logs/r11i/: every R11I transcript (matrix, regressions, equivalence, QEMU VGA, failure cases,
  fd-mode check, spare-span test, host tests).

Not included (deliberately)
- Emulator binaries (Tiny386, i386VM harnesses): identified by hash in docs/R11I-RUNTIME-REPORT.md and
  built from the historical emulator source and the patches.
- The private Zork I story volume and anything derived from it.
- The historical GCC 1.40 / GAS / i386-ld tree (hash in docs/R11I-BLOCK-AUDIT.md).

Launch
- Legacy RAM disk: i386vm-r9 build/i386vm-r11h-rel/qdos386-r11h-rel.bin
- Host-backed disk: i386vm-r9-block build/r11i/block/qdos386-r11i-block.bin build/fat16/win1-r11h.img

Verification
- docs/R11I-RUNTIME-REPORT.md.  SHA256SUMS lists every file in this package except itself.
