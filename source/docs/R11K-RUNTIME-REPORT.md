# R11K runtime report — bootloader expansion and practical paging integration

Branch `r11k-integration`, from frozen R11J `f7664d7` (R11I `bc9a90f`, R11H-FIX1 `4affc37` below it; none modified).
**Tested source commit: `e60219f9c187a64f129fc98592bde61e316346a2`.** This report and the final `PACKAGE.md` are
committed after it and change documentation only.

R11K is a bounded checkpoint. It is **not** a ring-3, security or isolation release (see `R11K-KNOWN-LIMITATIONS.md`).

Every claim below carries one label:

| Label | Meaning |
|---|---|
| **live-proven** | executed in the matrix on the named platform and judged from the transcript (`scripts/r11k-assert.py`) |
| **build-only** | built / linked / byte-compared, not executed for that claim |
| **code-reviewed** | read and argued, not executed |
| **blocked** | could not be done in this environment |
| **not tested** | not attempted |
| **NA** | the check cannot be evaluated on that platform; reported as NA, never counted as PASS |

## 1. Verdict

* **Part 1 (bootloader): delivered, live-proven.** `platform/pc/boot2.s` loads kernel + transfer area of up to 896
  sectors (the original loader stops at 128 sectors; its build policy at 120). A paging kernel of 122 sectors
  (62 464 bytes, more than the old 0xF000 policy margin) boots reliably on Tiny386 and QEMU/SeaBIOS.
* **Part 2 (paging launch path): delivered, live-proven** on i386VM (block mode), Tiny386 and QEMU, with the platform
  gaps listed in section 6. A launcher starts two copies of the same byte-identical slot-1 executable from the FAT16
  volume; they run concurrently under different CR3 values with independent writable images, use normal
  libc/libqdos calls, exit, release their resources and can be relaunched without a reboot.
* Matrix: **826 PASS, 0 FAIL, 18 NA** (`logs/r11k/matrix/assert.log`, `results.json`); boot matrix **204 PASS, 0 FAIL**
  (`logs/r11k/boot/matrix/`). Volume SHA256 unchanged by the whole matrix.

## 2. Identity of what was tested

| Item | SHA256 |
|---|---|
| Tiny386 headless (VGA-window build, from `fpu-diag`) | `e201502f21f488aa13e3d34dec0f9635d147d170fdfb057f44b15c070bbfbef9` |
| i386VM harness, legacy (RAM disk) | `a4724f79b26a716e64609399a8c199cbceaf98da9673d6010c41341766543e3a` |
| i386VM harness, block | `5970e16223075cf549ac30f643ad8fb508fda2fb4f9464c6ff6fa9b7117dd88d` |
| SeaBIOS used by QEMU (`/usr/share/seabios/bios.bin`) | `4d597f68e06a0e28498e12e96e1f2ce64aee88a519685c61da92eb775c95a445` |
| QEMU | 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), `-machine pc -cpu pentium -m 16 -vga std`, scripted keys through the monitor |
| Paging kernel, i386VM block | `2e61c4aee1d05e5dcb18fb40e9aec8ea068afa216e44099d310b583e9c83730a` |
| Paging image, PC (Tiny386) | `3a09141da237c85c1f44b9a843a72d6bb88baaecf1dfdfdbacdf74154129617f` |
| Paging image, PC VGA (QEMU) | `703ad9e28431783efb320eadd89c427b2b5673d6a67ccd40a976eb6ae5d23109` |
| Default image + new loader, PC / PC VGA | `2067b3e8…a202c` / `38404151…943e0` (full values in `SHA256SUMS`) |
| `boot2.bin` | `ca2b57e877f4116b2c431151634aec78ef2019e9dd01f5633a3dbd46952378c6` |
| TWICE / PGRUN / TWICET `.AOU` | `8724bd53…34730` / `76be89f0…33d8f` / `91e30bd6…bf59` |
| Test volume `win1-r11k.img` | `eabdc0fc9e47bc9c4dcb43fc6b7759e91481bad0d1502c56270589d87514c3e5` |

Toolchain: the historical GCC 1.40 / GAS 1.38 / i386-ld tree, unchanged; no new toolchain. `objdump` is used only by the
host test that compares the hand-encoded `boot2` against its reviewed disassembly.

**Volume hashes, before and after the entire matrix** (`logs/r11k/matrix/volumes-before.txt`, `volumes-after.txt`):
`win1-r11k.img` `eabdc0fc…c3e5`, `win1-r11h.img` `672806ad…477b`, `win1-r11h-neg.img` `5af61837…1ea` — identical (`VOLUMES UNCHANGED`).
R11K does not write the FAT volume.

**Default images are unchanged:** every non-paging image built with the original loader is byte-identical to its R11I /
R11H counterpart (`logs/r11k/default/identity.txt`: `IDENTICAL` ×5), and `SAME.AOU` / `SAMELNCH.AOU` are byte-identical to
their R11J files (`logs/r11k/default/sdk-identity.txt`). — **live-proven (byte comparison)**.

## 3. Part 1 — bootloader

Design and ABI: `R11K-BOOT-AUDIT.md`, `R11K-BOOT-ABI.md`. Chunked reads of at most 64 sectors into 0x10000–0x7FFFF,
BIOS memory-size check, CF/AH checked on every read, `cli` before the switch, A20 verified, rotate-add checksum, the
`QBOOT2` tag enforced by `make-image2.py`, diagnostics `QBOOT:N/M/R/A/S` on port 0x402, COM1 and VGA. Selected only by
`PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py`; the original loader remains the default and is a
build option, not tied to `QDOS_PAGING`.

| Claim | Label | Evidence |
|---|---|---|
| The original loader hangs silently at 129 sectors; 128 boot (the 64 KiB segment limit) | live-proven, Tiny386 + QEMU | `logs/r11k/boot/oldlimit-summary.txt` |
| New loader boots 119 / 120 / 121 / 128 / 129 / 200 / 400 / 896-sector images, default and paging, plain and VGA | live-proven, Tiny386 + QEMU, 3 rounds | `boot-matrix.txt` (204 PASS) |
| Kernel contents arrive intact (a 3.5–400 KB test payload `BIGK` is checksummed from inside the booted kernel) | live-proven | `got=PROMPT big=<n>` rows |
| Refusals: count zero / > 512 / sum > 896 / 16-bit wrap, truncated, corrupt, oversized, wrong tag | live-proven (images) + host test | `refuse-*`, `test_boot_images.py` (6 tests OK) |
| A refused image copies nothing to high memory | live-proven | `no-copy-after-refusal-*` rows |
| Truncation shows as `S` on Tiny386 (zeros past EOF) and `R` on QEMU | live-proven | both refuse |
| Machine code matches the reviewed disassembly | build-only (host test; needs `objdump`) | `boot2.golden.dis` |
| Real hardware, USB/CD/floppy, partitioned disks, BIOSes other than Tiny386's and SeaBIOS | **not tested** | |

## 4. Part 2 — paging launch path

Design: `R11K-PAGING-APP-ABI.md`, `R11K-MEMORY-MAP.md`, `R11K-PAGING-ABI-AUDIT.md`. Opt-in `-DQDOS_PAGING` (ABI 2);
libqdos member `qdospg.o` (`qd_pg_start`, `qd_pg_startv`, `qd_pg_cancel`, `qd_pginfo`, `qd_pg_caps`, `qd_pg_available`);
launcher `PGRUN.AOU`; acceptance program `TWICE.AOU`; the test driver `TWICET.AOU` (modes M H N E S T F W Z K Y X C).
Programs are linked once at 0x240000 with the slot-1 script; the same bytes run in every copy.

Platforms: **I** = i386VM block mode, **T** = Tiny386, **Q** = QEMU/SeaBIOS. 3 rounds each unless noted.

| Claim | Label | Platforms |
|---|---|---|
| Two copies of one byte-identical `TWICE.AOU` run concurrently under different CR3 and keep independent writable state (stack, `.data`, `.bss`, libc statics, qsort, malloc, sprintf, x87) | live-proven | I, T, Q |
| Overlap is certain, not probable: held start, group activation (`QD_PG_HELD`); `PGRUN: overlap yes` for TWICE and CATFIL | live-proven | I, T, Q |
| Two copies of a real SDK utility (`CATFIL`) read the same FAT16 file simultaneously (BIG.BIN checksum from both), both `FILECAT PASS`, exit 0 | live-proven | I, T, Q |
| Exit releases frames, slot and records (`released yes` after every `PGRUN` run); relaunch with no reboot | live-proven | I, T, Q |
| 10 launch/exit cycles (`TWICET S`) | live-proven | I, T (the QEMU sequence does not run it) |
| Repeat / stale ids / missing file / malformed args / unknown flag / wrong origin (−61) / bad header (−21) | live-proven | I, T |
| Exhaustion: held starts beyond the pool return −60 and leave nothing behind; group start is all-or-nothing | live-proven | I (7 windows), T (2 windows); not in the QEMU sequence |
| Failed copy (test-only kernel): atomic rollback | live-proven | I, T |
| Pool capped (test-only kernel `test-cap`) | live-proven | I |
| Paged caller cannot start programs (−62 on `QT_CREATE`, `POLL_ADD`, `SCHED_ADD`, `IMG_*`, `PG_START`, `PG_CANCEL`) | live-proven | I, T |
| Services from a paged process: file read, console, time, libc `malloc`/`printf`/`qsort`/x87 | live-proven | I, T, Q |
| Heap and stacks are **shared** (copy A's block is readable by copy B: `TWICET Z`) — a documented limitation, not a feature | live-proven | I, T, Q |
| Negative control: a kernel that shares the window frames makes the same acceptance test **FAIL** | live-proven (`control-share`, 3 runs, FAIL expected and observed) | I |
| A deliberate page fault in one copy ends only that job (−17); the other copies and the shell continue | live-proven | T, Q |
| Any exception 0–19 in a paged job's window is contained likewise | live-proven for #PF; other vectors **code-reviewed** | T, Q |
| The same fault test on i386VM | **NA** — the CPU core ends the run at the first guest exception (core not patched) | I |
| Mixed-CR3 hold flag and callbacks (interrupt taken around a CR3 switch; kernel poll / schedule tasks run while a paged copy is current) | live-proven (test-only kernels `intcr3`, `tasks`) | I, T |
| An unpaged task that faults while a paged copy is current **halts** the machine; it must not blame the paged copy | live-proven after the review fix (section 7) | T |
| Second `qd_con_wait` refused (−50); first released | live-proven | T, Q; **NA** on I (stdin already at EOF) |
| Non-paging kernels: every paging call answers `QD_PG_ERR_NOPAGING` (−70); `PGRUN` refuses cleanly; unchanged behaviour of MULTI/NEG | live-proven | I (block), T |
| Process-private heap | **not implemented** (the smallest bridge was not needed: ordinary apps run on the shared heap) | |
| Profile-2/3 and paged programs together on the PC/QEMU map | **code-reviewed** (the pools coincide, so they exclude each other); not run as a mixed test | |
| Zork I | **not tested** — the private story is not available; never bundled | |

Console behaviour is honest: one console, interleaved output, one input waiter (limitations 11–12).

## 5. Matrix and regressions

`scripts/r11k-matrix.sh` (+ `r11k-matrix-tail.sh`), judged by `scripts/r11k-assert.py`: PASS 826 / FAIL 0 / NA 18.

| Group | Platforms | Rounds | Result |
|---|---|---|---|
| Paging kernels: TWICET suite, PGRUN, SAME/SAMELNCH, R11H MULTI and NEG (848 checks, 0 failures) | I, T | 3 | PASS |
| Paging kernels, QEMU VGA scripted smoke | Q | 3 | PASS (3 NA per round: lines cut at 80 columns) |
| Large paging kernels (400- and 896-sector transfers) | T, Q | 1 | PASS |
| TEST-only kernels: failed copy (I, T), pool cap (I), interrupt around CR3 (I, T), kernel tasks (I, T), shared-frames control (I) | as listed; Q: images built, not run | 1 | PASS |
| R11F regression sequence (SDK programs, Phoon, time, FPU) under the paging kernel | I (compared line-by-line to the default kernel), T | 1 | PASS |
| Default (non-paging) images, original loader: legacy RAM-disk MULTI/NEG, block MULTI/NEG, PC MULTI/NEG | I legacy, I block, T | 3 | PASS |
| Non-paging sequence (`TWICET X`, `PGRUN` refusal): R11I block default; default image + new loader | I block; T | 3 | PASS |
| Default image + new loader, HELLO/CATFIL/MULTI | Q | 3 | PASS |
| R11H regression script on the default build (volume unchanged) | I | 1 | PASS |
| R11I host tests (`BLOCK: PASS`, `LAYOUT: 39 checks, 0 failures`); R11K host tests (6) | host | 1 | PASS |
| R11J `SAME` / `SAMELNCH` | I, T, Q | 3 | PASS |

The 18 NA: `TWICET W` on i386VM ×3 (stdin EOF), QEMU rows whose text is cut by the 80-column screen ×9, and 6 entries
for the short programs FORMAT / FPMULT / HELLO under `PGRUN` that finished before both copies could be sampled live
(`overlap no`). **No overlap claim is made for those; they are not counted as passes.** The programs' own results and
exit codes were checked.

The historical `floatundidf` discrepancies in the GCC 1.40 runtime were not touched and no expected result was changed.

## 6. What was not run, and why

* Real hardware and BIOSes other than Tiny386's and SeaBIOS: **not tested**.
* The deliberate-fault test on i386VM: **NA** (core limitation). On QEMU the page fault was observed in `TWICET F`
  (`PGFAULT: vec 0000000e … cr2 01000000 … paged job ended`, then `TWICET: page-fault copy ended with -17`, in all
  three QEMU rounds and in both large-kernel runs); the other exception vectors are not generated by any test.
* QEMU with the TEST-only kernels: not run (their images are built; the same behaviour is covered on Tiny386 and i386VM).
* Zork I: not tested (private story).

## 7. Independent review findings

A review pass over the fault path found, and I fixed:

1. **Wrong job blamed (reproduced before the fix).** An exception raised by an *unpaged* poll task while a paged copy's
   directory was current was attributed to that copy: `TWICET Y` showed `PGFAULT: paged job ended` for the innocent copy
   and then a second fault while handling the first (`logs/r11k/review/tiny386-prefix-Y.txt`). Fix: a job is ended only
   if the faulting EIP lies in its window **and** its own directory is the active CR3; otherwise the machine halts.
   After the fix the transcript is a single `PGFAULT: halt` with no job killed (`tiny386-fixed-Y.txt`; permanent matrix entry `pg-Y`). — **live-proven (Tiny386)**.
2. `cli` before the protected-mode switch in the loader; SDK maps a non-paging kernel to −70; `QBOOT2` tag enforced by
   `make-image2.py`; stale comments corrected. — host test / build-only, then exercised by the matrix.
3. Test-design flaw: `TWICE` first stored its identity in a window global, which shared frames would not reveal. It now
   derives it from `qd_self()`, which is what makes the negative control fail correctly.

## 8. Corrections made to the test tooling while finishing

Stated so that nothing is hidden:

* The first complete judging run reported **18 FAIL**. I checked each against the transcripts. None was a guest
  failure; all were wrong expectations in `r11k-assert.py`: (a) it required exit 0x41 and 0x42 and CATFIL on sequences
  that run only one of them (QEMU: 0x42 + CATFIL; large Tiny386: 0x41 only); (b) it counted `overlap no` of the short
  FORMAT / FPMULT / HELLO runs as a failure (now NA); (c) it looked for `PHOON … PASS` although PHOON legitimately prints
  `no clock is configured` — it now requires `PHCHK PASS`, `FPMULT PASS` and the two PHDATE epoch values
  (1704974220, 4134024000, both independently correct UTC values). The guest outputs were not changed.
* `r11k-qemu-capture.py` decided that a command had finished only when the prompt was on the **bottom screen row**. Short
  transcripts never scroll that far, so the default-kernel QEMU smoke waited out its 900 s limit per command
  (looking like a stall). It now uses the last non-empty row. The paging QEMU runs (long output) were unaffected; only
  the default-kernel QEMU smoke was re-run (`scripts/r11k-matrix-tail.sh`, with the regression, host tests and volume
  hashes); the rest of the matrix is from the single earlier full run.

## 9. Reproducing

`PACKAGE.md` has the rebuild steps; `python3 scripts/r11k-assert.py logs/r11k/matrix` re-judges the packaged transcripts;
`SHA256SUMS` lists every packaged file. The fresh-extract rebuild comparison (94 products, 0 differences) is in `logs/r11k/package-verify.txt`; checks of the final
tarball itself are in the sidecar `qdos386-R11K-PAGING-INTEGRATION.tar.gz.verify.txt` next to the archive.
