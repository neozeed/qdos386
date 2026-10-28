# qdos386-R11M-FIX1-TINY386-SHUTDOWN

Base: frozen R11M release `a16edcdea8ee25e24d89901a3f19e56bc2a72e07` (R11M-OS-USABILITY-STRETCH; R11L, R11K, R11J, R11I, R11H-FIX1 below it, unchanged).
Branch `r11m-fix1`.  Final source commit: `6fbb9ff7c8a695d91a368a985a2b1da8b53a9bbc`.  The SHA-256 of this archive is printed by the packaging script and reported with the
delivery (it cannot be inside the archive); `SHA256SUMS` lists every other file.

Read `docs/R11M-FIX1-RUNTIME-REPORT.md` first, then `docs/R11M-FIX1-DURABILITY-REPORT.md`, `docs/R11M-FIX1-SHUTDOWN-ABI.md`, `docs/R11M-FIX1-TEST-POLICY.md`.

Contents
- source tree (git archive of `6fbb9ff7c8a695d91a368a985a2b1da8b53a9bbc`): kernel (unchanged since R11M), SDK, shell, scripts (`scripts/fix1-*`, `scripts/legacy-i386vm.sh`), tests (`tests/fix1`),
  docs (`docs/R11M-FIX1-*.md`), `patches/tiny386-fix1-shutdown.patch`
- `emulator/`: the 7 patched Tiny386 source files, `bin/tiny386_headless` and `bin/tiny386` (windowed rawdraw frontend) built from base + patch in this
  environment (Linux x86-64), the Tiny386 licence, build notes
- `build/r11m/*`: the six R11M kernels (unchanged); `build/apps/shell-m.aout`; firmware `build/pc-r11f-none-run/{bios,vgabios}.bin`; `build/fix1/failsync.so` (fault-injection shim)
- `guest-programs/`: the R11M guest programs plus SHUTDOWN.AOU
- `volumes/fix1-initial.img`: the deterministic initial volume (R11M demo volume + SHUTDOWN.AOU); `volumes/r11m-demo-initial.img`: the unchanged R11M one
- `logs/fix1/`: transcripts, emulator end records (`*.end`), final images, fsck/fatwalk output, write traces, matrix and regression judges

Quick check: `sh scripts/fix1-smoke.sh OUT` (QEMU primary, Tiny386 with guest-requested shutdown; i386VM not run).
