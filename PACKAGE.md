# qdos386-R11L-SELFHOST-STRETCH

Base: frozen R11K `82405025f873dcf13dea8bf352399add195ebaca` (R11J `f7664d7`, R11I `bc9a90f`, R11H-FIX1 `4affc37` below it, unchanged).
Branch `r11l-selfhost`.  Final source commit: `81d29ea900b60955837cbb394ea6176e9ffa9fa1`.

Native, historical GCC 1.40 (cccp, cc1), GNU as 1.38 and the binutils-1.94 i386 a.out ld, running as QDOS/386 programs on a
writable FAT16 volume, driven by `CC.AOU`.  Read `docs/R11L-SELFHOST-RUNTIME-REPORT.md` first, then
`docs/R11L-KNOWN-LIMITATIONS.md` for the exact boundary of the claim (the tools themselves were built on the host).

Contents
- source tree (git archive of `81d29ea900b60955837cbb394ea6176e9ffa9fa1`): kernel, platform, SDK (libc additions, `sdk/examples/r11l`), scripts, tests, docs (`docs/R11L-*.md`)
- `toolchain/`: original legacy sources (LF copy) and the guest-staged copy, SHA-256 of every original file, host bootstrap
  binaries, the LP64-repaired host cc1, the guest tools `AS/CCCP/CC1/LD.AOU` and utilities, the compiled objects
- `sdk/lib/*.a`, kernels (`build/r11l/...`), the writable harness binary, `build/apps/shell-l.aout`
- `logs/r11l/`: oracle (host) artifacts, Stage 1-5 transcripts, QEMU screen captures, Tiny386 transcripts, scratch disk images
  (initial and final), `fsck.fat` and `tools/r11l-fatcheck.py` reports, regression matrices and their judge output
- `artifacts/hello/`: HELLO.C .I .S .O .AOU and CRT0.O as left on the final scratch volume by the guest

Not included: emulator binaries (Tiny386, QEMU), the private Zork I story.
`SHA256SUMS` lists every file of the package except itself.
