# qdos386-R12A-DYNAMIC-PROCESSES

Base: frozen R12 `0e92a742291a47067823d42783ee3eb366d02b83`. Branch `r12a-dynamic-processes`. Final source commit: `6e30dd7046b4d09d77ec398bea38a76318b36c52`.
The SHA-256 of this archive is printed by the packaging script and reported with the delivery (it cannot be inside the
archive); `SHA256SUMS` lists every other file.

Read `docs/R12A-RUNTIME-REPORT.md` first, then `docs/R12A-KNOWN-LIMITATIONS.md`, `docs/R12A-PROCESS-ABI.md`,
`docs/R12A-MEMORY-MAP.md`, `docs/R12A-INTERRUPT-STACKS.md`, `docs/R12A-MEMORY-AUDIT.md`, `docs/R12A-MIGRATION-DESIGN.md`.

**Private process memory is not security isolation: everything still runs at CPL 0.**

Contents
- source tree (git archive of `6e30dd7046b4d09d77ec398bea38a76318b36c52`): kernel (`kernel/proc.c vas.c frames.c pheap.c kctx.s` are new), SDK
  (`sdk/lib/qdosproc.c`), tests (`tests/r12a`, `tests/host/r12a`), docs
- `build/r12a/pc-vga/r12a-vga.img` (QEMU kernel), `build/r12a/pc/r12a.img` (headless/Tiny386 kernel); the unchanged R12 kernels
  `build/r12/pc-pg*/` for comparison; firmware in `build/pc-r11f-none-run/`
- `guest-programs/`: R12A tests (MAPTEST, BIGBSS, HEAPT, STKT, FAULTT, SPAWNT, CHILDT) and the unchanged R12 programs
- `volumes/r12a-initial.img`, `volumes/r12a-demo-initial.img` (adds the SuperBASIC corpus)
- `emulator/`: patched Tiny386 sources and headless binary (from R11M-FIX1)
- `logs/r12a/`: per-stage transcripts, judge output, final disk images

Launch: `sh scripts/r12a-launch-qemu.sh`.  Host tests: `sh tests/host/r12a/run.sh`.
Gates: `sh tests/r12a/run-s1.sh OUT` … `run-s3.sh`, `SB4=... sh tests/r12a/run-demo.sh OUT`.

SuperBASIC is private research code and not freely redistributable; `apps/superbasic/docs/PROVENANCE-LICENSING.md` and
`SHA256SUMS.sb4` are shipped unchanged in meaning.
