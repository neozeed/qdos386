# R12 known limitations and NOT TESTED list

Everything here is a stated limit, not a hidden failure. Evidence classes: **host build**, **guest executed** (QEMU full machine unless stated), **independent comparison** (mtools, `fsck.fat`, `tools/r11m-fatwalk.py --vfat`, supplied SuperBASIC host runner), **not tested**, **blocked/deferred**.

## Deferred / not provided
* **Stage 5 (graphics): deferred, Gate 5 not claimed.** Design only: `R12-GRAPHICS-POC.md`. The release is text mode.
* **No independent shells.** One shell on console 0; applications on the other consoles. A paged job cannot start, wait for or kill jobs.
* **At most three paged applications at once** (`QDOS_PG_WIN3` kernels; R11M kernels: two). The fourth `START` is refused with "no free image window".
* **Shell `EXIT` stops the whole session** (kernel reports the shell's exit); R12 only adds a one-time warning when started jobs are running.
* **Console:** four 80x25 text consoles; no active-console status bar; no scroll-back; no mouse; raw key codes 0x80..0x99 are the whole key set.
* **Filesystem (unchanged from R11M):** FAT16 only, VFAT long names up to 63 characters, writes at most 256 bytes per call, 16 channels in the system (15 free at idle). No power-cut atomicity claim; no write-protected-volume test on QEMU.
* **Editor:** 262,144-byte gap buffer (larger files refused, not truncated); no undo, mouse or regex; binary (NUL) files refused; a killed editor does not save. See `R12-CONSOLE-EDITOR-ABI.md`.

## SuperBASIC (`BASIC.AOU`)
* Private research code, **not freely redistributable**; the provenance notice and the SB4 `SHA256SUMS` are shipped with the core (`apps/superbasic/`). This package makes no redistribution claim. The core is byte-identical to the supplied files except `sb_state.h` (a compile-time hook for the call-depth limit); `apps/superbasic/verify-core.sh` shows it.
* Nested `DEFine` depth **156** at the default 256 KiB stack (`/M:kib` changes it, capped at 384 KiB); the ROM went past 5000. Lines are limited to 255 characters. Break (Ctrl+C) works only while a program prints; otherwise `KILL /CON:n` or the `/L:steps` safeguard.
* The 256 KiB interpreter stack comes from the one ~0.9 MB shared kernel heap (idle free 667,024 bytes).
* The supplied core has no `OPEN`/`CLOSE`/`SAVE`/`LOAD`, no graphics, `INPUT "prompt";x`, or most QL built-ins (error -19/-17). 13 of 14 "known" fixtures still differ from the oracle, as the supplied package documents.
* Not a QL ROM or QDOS system-call implementation.

## Tiny386
* One bounded headless smoke only (guest-triggered `0x8900` shutdown). The primary gate is QEMU.
* `SHUTDOWN.AOU` refuses while another started job runs (by design); the smoke kills its ticker first.

## NOT TESTED
* Write-protected volume on QEMU; power-cut behaviour; reboot after a kill during a write.
* Killing a job while the shell is itself blocked in `WAIT`.
* The cooked-input fallback of `BASIC.AOU` on an R11M kernel (no console key API).
* BASIC on Tiny386 beyond the single smoke line (`times_table.bas`); the editor on Tiny386.
* Kernel-heap exhaustion with BASIC plus three other jobs; behaviour with a damaged BASIC stack guard (never observed).
* A program that nests long parenthesised expressions inside every call level.
* Zork (optional in the plan).
* CPU-only i386VM (retired by the test policy), guest-native GCC/GAS/ld rebuild (out of scope).
