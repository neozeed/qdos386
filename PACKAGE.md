# qdos386-R12-INTERACTIVE-OS-STRETCH

Base: frozen R11M-FIX1 `6fbb9ff7c8a695d91a368a985a2b1da8b53a9bbc`. Branch `r12-interactive`. Final source commit: `0e92a742291a47067823d42783ee3eb366d02b83`.
The SHA-256 of this archive is printed by the packaging script and reported with the delivery (it cannot be inside the archive);
`SHA256SUMS` lists every other file.

Read `docs/R12-RUNTIME-REPORT.md` first, then `docs/R12-KNOWN-LIMITATIONS.md`, `docs/R12-AUDIT.md`, `docs/R12-SHELL-ABI.md`,
`docs/R12-CONSOLE-EDITOR-ABI.md`, `docs/R12-SESSIONS-JOBS.md`, `docs/R12-SUPERBASIC-PORT.md`, `docs/R12-GRAPHICS-POC.md` (Stage 5 deferred).

Contents
- source tree (git archive of `0e92a742291a47067823d42783ee3eb366d02b83`): kernel, SDK, shell, editor, SuperBASIC port (`apps/superbasic`), scripts (`scripts/r12-*`), tests (`tests/r12`), docs
- `build/r12/pc-pg-vga/r12-pg-vga.img`: the QEMU kernel; `build/r12/pc-pg/r12-pg.img`: the headless/Tiny386 kernel; firmware in `build/pc-r11f-none-run/`
- `guest-programs/`: BASIC, EDIT, SDK utilities, Phoon (.aout; the volume names them .AOU)
- `volumes/r12-initial.img` (shell + editor + tools), `volumes/r12-s4-initial.img` (adds the SuperBASIC corpus and DEMO.BAS)
- `emulator/`: the patched Tiny386 sources and headless binary (Linux x86-64) from R11M-FIX1
- `logs/r12/`: per-stage and integrated transcripts, judge output, final disk images, fsck/fatwalk results

Launch: `sh scripts/r12-launch-qemu.sh` (interactive QEMU; Alt+F1..F4 switch consoles; `DIR /W`, `EDIT`, `START BASIC.AOU /CON:2`).
Demo check: `sh tests/r12/run-demo.sh OUT` (needs QEMU, mtools, fsck.fat, SB4).

SuperBASIC is private research code and not freely redistributable; `apps/superbasic/docs/PROVENANCE-LICENSING.md` and `SHA256SUMS.sb4` are shipped unchanged in meaning. Nothing here is a redistribution grant.
