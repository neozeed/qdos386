# R11M-FIX1 runtime report: Tiny386 clean shutdown and emulator policy

Base: the frozen R11M release `a16edcdea8ee25e24d89901a3f19e56bc2a72e07` (unchanged; branch `r11m-fix1`). Date of the runs: 28 October 2026.
**Source commit tested:** the final commit named in `PACKAGE.md` and in the delivery message (the report cannot contain its own hash).
Verification environment: one Linux x86-64 container with 2 cores; QEMU 8.2.2; `fsck.fat` 4.2; mtools; Xvfb + xdotool.

## 1. What changed

| Area | Change |
|---|---|
| Tiny386 (external tree, `patches/tiny386-fix1-shutdown.patch`, 7 files) | checked flush / fsync / close of every writable disk image at the end of the run (`ide_flush_all`, `pc_shutdown`), exit status 3 on failure; all frontends end through it |
| Request from the guest | the emulator's existing "Shutdown" string on port 0x8900 (no new port; audit and reasoning in `docs/R11M-FIX1-SHUTDOWN-ABI.md`) |
| Guest | `SHUTDOWN.AOU` (`sdk/examples/r11m/shutdown`): refuses while launched jobs exist, flushes stdio, writes the string; **no kernel change** |
| Tests | `scripts/fix1-*.sh`, `tests/fix1/*`, termination modes in `tests/interactive/run_pc_multi.py` (`PC_END=kill|term|wait`, default `kill` = unchanged) |
| Policy | QEMU primary, Tiny386 secondary, CPU-only i386VM legacy/optional (`docs/R11M-FIX1-TEST-POLICY.md`) |

Not changed, and verified so: the six R11M kernels and `shell-m.aout` are byte-identical to the R11M release (SHA-256 compared against the R11M `SHA256SUMS`); the six default images are IDENTICAL to
their frozen hashes (`scripts/r11l-default-check.sh`: r11i-block, r11k-vm-paging, r11k-pc-default, r11k-pc-paging, r11h-pc, r11h-rel-vm). FAT16/VFAT, paging, job control, virtual consoles, GCC, the CPU core: untouched.

## 2. Acceptance

| Acceptance condition | Result |
|---|---|
| An R11M guest on full-machine Tiny386 writes files, explicitly requests a clean emulator shutdown, the emulator flushes/closes its disk images and exits | **PASS** (section 3; 12 of 12 boots in the matrix, plus the demo, the smoke and the windowed run) |
| The resulting FAT16/VFAT image passes independent checks after reboot **without a dummy read** | **PASS**: FAT copies equal, `fsck.fat -n` clean, `r11m-fatwalk.py --strict --vfat` 0 errors, mtools content compared; every boot was a fresh process on the previous image |
| QEMU remains the primary release gate and CPU-only i386VM is no longer mandatory | **PASS** as policy and scripts (`scripts/fix1-gate.sh` runs no i386VM); i386VM was **not run** in this milestone |
| Old kill-without-flush case | **still fails by design**, labelled a forced-power-loss limitation; reproduced in 4 of 4 forced-kill runs, in no guest-ended or SIGTERM run |
| Crash-safe writes | **NOT CLAIMED** |

## 3. Tiny386 full-machine headless (patched emulator), shutdown/persistence matrix

`scripts/fix1-matrix.sh` (judge `logs/fix1/matrix/judge.txt`, **63 checks, 0 FAIL**, wall time about 12 minutes): three independent rounds in parallel, each four boots on one evolving
volume, **every boot a fresh emulator process ended by `RUN SHUTDOWN.AOU`**, no extra read:

| Boot | Content | Checks (per round) |
|---|---|---|
| b1 | `PS`, `JOBT` (69 checks; WRITER killed, its chain freed in both FAT copies), `PS`, shutdown | exit 0 by itself; flush/close reported; FAT copies equal, fsck.fat, fatwalk; JOBT banner and 69/69 |
| b2 | VFAT s4a: `MD`, long names, copies, renames, long directory, shutdown | the above plus alias/long-name listings |
| b3 | VFAT s4b after reboot: read by long name and alias, rename, copy, shutdown | the above plus listing after reboot |
| b4 | VFAT s4c: delete everything, `RD`, shutdown | the above plus mtools: SRC gone, unrelated files intact |

Negative and forced cases (12 checks, same judge):

| Case | Result |
|---|---|
| wrong magic (`SHUTDOWN /W`) | **PASS**: ignored, shell still answers `PS`, the real request afterwards ends the emulator with status 0 |
| shutdown while a started job exists | **PASS**: refused; after `KILL` it succeeds, image clean |
| shutdown right after boot (no writes) | **PASS**: status 0, volume byte-identical to the initial volume |
| injected fsync EIO / injected fflush EIO | **PASS** both: exit by itself with **status 3**, "FLUSH FAILED" and the failing step on stderr |
| SIGTERM after JOBT | **PASS**: status 0, image clean |
| SIGKILL after JOBT (forced) | information: image **differs in one FAT entry** (the original finding), as expected |
| injected EFBIG (RLIMIT_FSIZE) | information only: the emulator's stream goes into error state and later guest reads return garbage; pre-existing, not judged (`logs/fix1/fsize-injection`) |
| no writable disk | **NOT TESTED** (the BIOS cannot boot without a writable `hda`; the loop over zero registered images is trivial) |
| read-only disk (CD-ROM image) | **NOT TESTED** (code path: read-only images are never registered) |

## 4. Kernel and programs still work on the patched emulator

| Check | Where | Result |
|---|---|---|
| R11M demonstration (DIR, long names, consoles 1 and 2 with focus switching and background progress, PS/KILL, CATFIL, Phoon, JOBT; phase B after reboot), **both phases ended by the guest** | Tiny386 patched, `scripts/fix1-demo-pc.sh` | **PASS** (29 judge lines ok; `FIX1 DEMO pc PASS`) |
| Earlier rounds' sequences on the R11M kernels (MULTI, NEG, PGRUN/paging, R11F regression with Phoon/PHCHK, non-paging default kernels) | Tiny386 patched, one round | **PASS**: 96 checks PASS, 1 NA (short PGRUN programs), 0 FAIL |
| the same sequences | QEMU VGA (primary) | **PASS**: 54 checks PASS, 3 NA (80-column cut), 0 FAIL |
| frozen read-only volumes | `scripts/fix1-regress.sh` | UNCHANGED (hashes before/after) |
| R11M kernels, shell, defaults | byte comparison | identical to the R11M release; 6 of 6 default hashes IDENTICAL |

`scripts/fix1-regress.sh` runs **no** i386VM groups; `tests/fix1/regress_filter.py` classifies the groups it does not run (i386VM, large-transfer loaders, test kernels, host tests) as NOT RUN, and reports
**real failures: 0**.

## 5. Windowed Tiny386 (rawdraw frontend) under Xvfb

`scripts/fix1-window-smoke.sh` (xdotool types into the X window; timing based): **PASS, 6 checks**. (a) `RUN SHUTDOWN.AOU` typed in the window: exit 0, "guest shutdown request; disk images flushed and closed",
image clean. (b) SIGTERM: exit 0, "stopped; disk images flushed and closed" (this run wrote nothing, so the image check is trivially clean). It exercises `pc_shutdown()` from the
windowed build; it is a smoke test, not a matrix.
The SDL frontend carries the same change but was **not compiled** here (no SDL development files): **NOT TESTED**.

## 6. QEMU

* QEMU 8.2.2 does **not** act on the 0x8900 string (`SHUTDOWN: requesting` then `SHUTDOWN: no response`, status 2; `logs/fix1/qemu-port-8900`). QEMU runs continue to be ended by the harness through the monitor; QEMU's raw backend writes straight to the host
  file, and no QEMU image in R11M or FIX1 showed the mismatch. The required QEMU path contains nothing Tiny386-specific.
* Control smoke from the freshly extracted package (`scripts/fix1-smoke.sh`, `logs/fix1/qemu-control-smoke.log`): **PASS**. SHA256SUMS (2,247 files) verified; QEMU: VFAT s4a then s4b after a reboot, long name and alias listed, FAT copies equal,
  `fsck.fat` clean, `fatwalk --strict --vfat` 0 errors, mtools copy identical; Tiny386 (the packaged patched `emulator/bin/tiny386_headless`): s4a and s4b each ended by `SHUTDOWN.AOU`, exit 0, image clean,
  listing after reboot, flush report, and a shutdown right after boot leaves the volume byte-identical; i386VM NOT RUN. The smoke ran on the archive built from commit `564dabd`; the final archive differs from it
  by this paragraph, the smoke log and the evidence summary.

## 7. Explicit labels

| Item | Label |
|---|---|
| QEMU VFAT/regression/control | PASS (primary) |
| Tiny386 headless, guest-ended persistence, 3 rounds x 4 boots + negative cases | PASS |
| Tiny386 headless, demonstration, regression (one round) | PASS |
| Tiny386 windowed rawdraw, smoke | PASS (smoke only) |
| Tiny386 SDL frontend | NOT TESTED (not compiled) |
| CPU-only i386VM | NOT RUN (legacy diagnostic; frozen results kept) |
| Forced kill / power loss durability | FAIL by design, documented limitation |
| Crash-safe writes, ATA FLUSH CACHE, mount-time FAT-copy comparison | NOT PROVIDED (storage-hardening milestone) |
| Real hardware, FAT32, non-ASCII long names, native GCC in the guest | NOT TESTED (out of scope) |

## 8. Reproduce

```
export QDOS386_TOOLS=<legacy tree>            # as for R11M
make r11m-kernels && make -C sdk/examples SHUTDOWN.aout VFATT.aout JOBT.aout ...    # as scripts/r11m-package.sh
TINY386=<patched tiny386_headless> sh scripts/fix1-matrix.sh OUT 3                    # about 12 minutes on 2 cores
TINY386=<patched tiny386_headless> sh scripts/fix1-demo-pc.sh OUT
TINY386=<patched tiny386_headless> sh scripts/fix1-regress.sh OUT                     # QEMU + Tiny386
TINY386_WIN=<patched tiny386> sh scripts/fix1-window-smoke.sh OUT
TINY386=<patched tiny386_headless> sh scripts/fix1-gate.sh OUT all                    # the bounded default gate
sh scripts/fix1-smoke.sh OUT                                                          # from an extracted package
```
Evidence: `logs/fix1/SUMMARY.txt` and the sub-directories named in `docs/R11M-FIX1-DURABILITY-REPORT.md`.
