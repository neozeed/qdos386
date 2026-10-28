# qdos386-R11K-PAGING-INTEGRATION

Base: frozen R11J `f7664d7` (archive SHA256 `24c5c635...`; R11I `bc9a90f` and R11H-FIX1 `4affc37` below it, all unchanged).
Branch `r11k-integration`.  Final source commit: `82405025f873dcf13dea8bf352399add195ebaca`.

**Bounded checkpoint, not an isolation release.**  Ring 0 only; only the 1 MiB image window of a paged
process is private; the heap, stacks and kernel are shared.  Paging exists only in images built with
`-DQDOS_PAGING`; every default image is byte-identical to R11I (`logs/r11k/default/identity.txt`).  The new
PC loader (`boot2`) is selectable independently of paging; the original loader is still the default.

Contents
- Source tree (git archive of `82405025f873dcf13dea8bf352399add195ebaca`): kernel, platform (incl. `platform/pc/boot2.s`, `make-image2.py`), SDK
  (`qdospg.c`, `examples/r11k`), tools, scripts, docs (`docs/R11K-*.md`), tests, patches, logs.
- `build/r11k/`: paging kernels (i386VM block; PC; PC VGA), default kernels with the new loader, TEST ONLY kernels
  (failed copy, pool cap, interrupt around CR3, kernel tasks, shared-frames negative control), bootloader boundary
  disks (`bootimg/`), `apps/` (TWICE, PGRUN, TWICET, SAME, SAMELNCH), `vol/BIG.BIN`.
- `build/fat16/win1-r11k.img`: R11K test volume = the frozen R11H release files + R11J and R11K programs + BIG.BIN.
- Everything carried forward from R11J's package `build/` (default images, R11H volumes, firmware, harness binaries).
- `logs/r11k/`: baseline, audit, default identity, boot matrix (3 rounds), old-limit experiment, full matrix
  (3 rounds each), `results.json` from `scripts/r11k-assert.py`.

Not included (deliberately): emulator binaries (Tiny386, QEMU) and the historical GCC 1.40 tree (identified by
hash in `docs/R11K-RUNTIME-REPORT.md`); the private Zork I story.

Rebuild (fresh extraction)
    export I386VM=<historical tree> QDOS386_TOOLS=<same tree>
    sh scripts/r11j-build-harness.sh build/harness
    sh scripts/r11k-images.sh            # paging, default-with-new-loader and TEST kernels
    sh scripts/r11k-boot-images.sh       # bootloader boundary disks
    sh scripts/r11k-build.sh             # SDK libraries, TWICE/PGRUN/TWICET, win1-r11k.img

Verification: `docs/R11K-RUNTIME-REPORT.md`; `python3 scripts/r11k-assert.py logs/r11k/matrix`; `SHA256SUMS` lists
every file in this package except itself.
