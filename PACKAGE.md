# qdos386-R11M-OS-USABILITY-STRETCH

Base: frozen R11L `81d29ea900b60955837cbb394ea6176e9ffa9fa1` (R11K `82405025`, R11J `f7664d7`, R11I `bc9a90f`, R11H-FIX1 `4affc37` below it, unchanged).
Branch `r11m-os-usability`.  Final source commit: `a16edcdea8ee25e24d89901a3f19e56bc2a72e07`.  The SHA-256 of this archive is printed by the packaging script and reported
with the delivery (it cannot be inside the archive); `SHA256SUMS` lists every other file.

Read `docs/R11M-RUNTIME-REPORT.md` first, then `docs/R11M-KNOWN-LIMITATIONS.md` for the exact boundary of the claim.

Contents
- source tree (git archive of `a16edcdea8ee25e24d89901a3f19e56bc2a72e07`): kernel, platform, SDK headers and sources (`sdk/include`, `sdk/lib`, `sdk/libc`, `sdk/examples/r11m`),
  shell (`apps/shell`), image builders and scripts (`scripts/r11m-*`), host tests (`tests/host/r11m`), guest sequences (`tests/r11m`),
  tools (`tools/r11m-fatwalk.py`), docs (`docs/R11M-*.md`)
- `sdk/lib/*.a`: the libraries the guest programs are linked against
- `build/r11m/*`: the six R11M kernels (i386VM plain/paging, Tiny386 plain/paging, QEMU VGA plain/paging); `build/apps/shell-m.aout`; the writable block harness
- `guest-programs/`: CONT1, COUNTER, TICKER, CONINP, JOBT, WRITER, PATHT, DIRFUL, VFATT, PHOON (host cross-built, run in the guest)
- `volumes/`: the deterministic initial scratch volumes (the originals were never written; every run used a copy)
- `logs/r11m/`: runtime transcripts, QEMU VGA captures, Tiny386 transcripts, final scratch images, `fsck.fat` / `r11m-fatwalk.py` /
  mtools judge output, matrix rounds, host test logs, frozen-hash checks

Not included: emulator binaries (Tiny386, QEMU), the private Zork I story (none was provided or invented).
