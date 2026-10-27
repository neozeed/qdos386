# qdos386-R11J-PAGING-POC

Base: frozen R11I `bc9a90f` (archive SHA256 `fb7b341495d0cd1fe6ec75649b163733010f82e7e5164f11259777b871539924`),
itself on frozen R11H-FIX1 `4affc37`; both unchanged.  Branch `r11j-paging`.  Final source commit: `f7664d7`.

**Proof of concept.**  Paging exists only in images built with `-DQDOS_PAGING_POC`.  Every default
image is byte-identical to R11I.  Ring 0 only; no memory protection.

Contents
- Source tree (git archive of `f7664d7`): kernel (incl. `kernel/paging.{c,h,s}`), platform, SDK, tools,
  scripts, docs, tests, patches, third_party, logs.
- `patches/i386vm-faultdump.patch`: optional harness-only (machine.c) CPU-state dump for i386VM faults.
  The R11I patches and harness sources are unchanged.  The CPU core is not patched.
- `build/r11j/vm-poc/qdos386-r11j-poc.bin`: POC kernel-only image, i386VM block mode.
- `build/r11j/pc-poc/r11j-poc.img`, `build/r11j/pc-poc-vga/r11j-poc-vga.img`: POC PC disks (Tiny386; VGA for QEMU).
- `build/r11j/test-*`: TEST ONLY images (failed copy, 2 MiB pool cap, interrupt around every CR3 load, and the shared-frames NEGATIVE CONTROL `test-vm-share`, which must FAIL).
- `build/r11j/apps/SAME.AOU`, `SAMELNCH.AOU`: proof programs (unchanged SDK, slot 1).
- `build/fat16/win1-r11j.img`: R11J test volume = the R11H release volume's files + the two programs.
- Everything from the R11I package's `build/` (default images, R11H volumes, firmware), unchanged.
- `logs/r11j/`: baseline, matrix (3 rounds; QEMU 3 rounds), test images, `matrix/results.json`.

Not included (deliberately)
- Emulator binaries (Tiny386, i386VM harnesses, QEMU) and the historical GCC 1.40 tree: identified by
  hash in `docs/R11J-PAGING-RUNTIME-REPORT.md`.
- The private Zork I story.

Rebuild (fresh extraction)
    export I386VM=<historical tree> QDOS386_TOOLS=<same tree>
    sh scripts/r11j-build-harness.sh build/harness
    sh scripts/r11j-images.sh        # POC and TEST kernels
    sh scripts/r11j-build.sh         # SDK libraries, SAME/SAMELNCH, win1-r11j.img (checks R11H reproduction)

Verification
- `docs/R11J-PAGING-RUNTIME-REPORT.md`: live-proven / build-only / not tested, per platform.
- `scripts/r11j-assert.py logs/r11j/matrix`: re-judges every transcript (375 PASS, 0 FAIL, 9 NA).
- `SHA256SUMS` lists every file in this package except itself.
