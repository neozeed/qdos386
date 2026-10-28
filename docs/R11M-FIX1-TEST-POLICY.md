# R11M-FIX1 test policy: which emulator is for what

Effective from R11M-FIX1. It records the decision made at the end of R11M (QEMU is the integration and regression environment; the slow CPU-only and
interpretive runners are not release gates) and the bounded way each target is now run. Frozen milestones (R11H to R11M) keep their original
results and are not invalidated; they stay labelled with the configuration they were run on.

## Roles

| Target | Role | What it is | Default run |
|---|---|---|---|
| **QEMU full machine** (`qemu-system-i386 -machine pc`, VGA text capture through the monitor) | **primary** release gate: integration and regression | real firmware, PIIX IDE, VGA | yes |
| **Tiny386 full machine** (patched `tiny386_headless`; windowed rawdraw frontend as smoke) | **secondary** compatibility target; the only one that needs the shutdown request to persist its disk | interpretive PC emulator | yes, focused (one round) |
| **CPU-only i386VM harness** (`i386vm-r9-blockrw`, no BIOS, no devices) | **legacy diagnostic, archived, optional** | the original bring-up harness | **no**: only `scripts/legacy-i386vm.sh` |

The i386VM sources, `patches/i386vm-*.patch`, the harness and every historic log are kept. Nothing is deleted and no earlier result is withdrawn.

## What is mandatory now

`sh scripts/fix1-gate.sh OUT all` (or the parts: `qemu`, `tiny386`):

1. QEMU: `scripts/r11m-vfat-gate.sh qemu` (create, reboot, read, rename, copy, reboot, delete; fsck.fat, fatwalk, mtools judge each image).
2. Tiny386: `scripts/fix1-matrix.sh` with `FIX1_ROUNDS` (default 1; the release evidence used 3): each boot of a round is a **fresh emulator process on the previous
   image, ended by the guest** (`RUN SHUTDOWN.AOU`), then FAT copies / fsck.fat / fatwalk, plus the negative and forced cases.
3. Regression of the earlier rounds: `scripts/fix1-regress.sh` (QEMU groups and the Tiny386 groups; **no** i386VM groups), judged by `scripts/r11k-assert.py` and
   `tests/fix1/regress_filter.py`.
4. Windowed Tiny386: `scripts/fix1-window-smoke.sh` (Xvfb + xdotool), where X is available.
5. Packaged-tree smoke: `scripts/fix1-smoke.sh`.

Not part of any of them: `scripts/r11m-matrix.sh vm`, `scripts/r11m-regress.sh`'s `vmb` groups, `scripts/r11m-smoke.sh`'s i386VM phase, any three-round matrix on
an interpretive target. Those R11M scripts are historic, still work, and are not referenced by the gate. No test requires full guest-native GCC compilation.

## Bounds

* every Tiny386 command has a timeout (`PC_CMD_TIMEOUT`, default 900 s in the historic runners; the FIX1 scripts keep it but end the emulator within `PC_END_WAIT`, 30 s);
* the end of a run is a recorded state (`*.end`: `mode`, `exited`, `rc`), not a sleep; a guest-ended run that does not exit is a FAIL and is killed;
* each script prints one completion marker (`FIX1 TINY386 MATRIX PASS`, `FIX1 WINDOW SMOKE PASS`, `FIX1 SMOKE PASS`, `FIX1 GATE PASS`) and nothing is reported PASS that did not run;
* measured cost on this 2-core host: Tiny386 matrix (3 rounds x 4 boots in parallel + negative cases) about 12 minutes; the QEMU control smoke about 4 minutes.

## Which sequences end how

| End | When to use | Image state |
|---|---|---|
| `!RUN SHUTDOWN.AOU` + `PC_END=wait` | any run whose final image is judged | flushed, synced, closed by the emulator |
| `PC_END=term` | emulator without a guest (diagnostic), headless/rawdraw only | flushed (signal handler leaves the loop) |
| `PC_END=kill` (the historic default of `run_pc_multi.py`) | read-only volumes, transcripts only | **may lose the last buffered sector write**; never judge a written image after it |

`scripts/r11m-run-pc.sh` (the extra-read workaround) is not used by anything in FIX1 and stays only for the frozen R11M evidence.
