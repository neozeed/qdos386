# R11H runtime report

Branch `r11h`. Phase 2 checkpoint: commit a3c432a (not rewritten). This report covers the work after it.
Scope: fixed-origin profiles, guest negative tests, memory-map verification, regression, QEMU VGA
smoke, packaging. Not in scope and not done: executable relocation, FAT32, virtual framebuffers,
the synthetic i386VM block device, and shell START/PS (deferred; see docs/R11H-EXEC-ABI.md section 7).

## 1. Memory-map verification

`scripts/r11h-memmap-check.py` reads the reservations from appmap.h (recursive macro expansion), reads
`_bss_end` from the kernel a.out for each target, and checks reservations, overlaps, the stack top,
pool spans inside each profile, and the absence of profiles 2 and 3 on the default map.

Result: `MEMMAP CHECK: PASS` (tiny386, i386vm R9, i386vm R8).

Limit: the stack top 0x7FFFF0 is verified for the PC target only. The i386VM initial ESP is not
verified in source (docs/R11H-MEMORY-MAP.md, open items).

## 2. Guest negative tests (NEG)

`sdk/examples/r11h/neg.c` runs 848 checks: malformed a.out table (13 cases, expected loader codes), a
good pair after refusals, contention with two live children (COLLIDE@2, MULTIB@3 and MULTI@1 give
-28), HELLO (slot-1 image) on profile 0 gives -33, 40-cycle churn with the reuse rule over all older
ids (780 stale ids refused), and invalid ids (0, -1, 0x7FFF0001) give -2.

Harness results (matrix, `scripts/r11h-matrix.sh`, 3 rounds per emulator):

| Target | Clock | Result |
|---|---|---|
| Tiny386 | default | 848 / 0 failures, PASS x3 |
| i386VM | default | 848 / 0 failures, PASS x3 |
| Tiny386 | pinned | 848 / 0 failures, PASS |
| i386VM | pinned | 848 / 0 failures, PASS |

### QEMU VGA NEG failure: resolved, test-setup error

The first QEMU VGA smoke run printed `NEG: checks 848, failures 10` and `NEG: FAIL`. The failing
lines had scrolled off the 25-line console. The harness runs had no failures, so the cause was the
inputs, not the loader or timing.

Capture method: `scripts/r11h-qemu-capture.py` drives QEMU through its monitor and reads the VGA text
buffer (0xB8000) in a tight loop, keeping every changed screen. Its output for the old run
(`evidence-qemu-neg/qemu-vga-neg-old-zork-volume-capture.txt`) shows the ten failures are all
malformed-image cases, each returning -44 (FAT_ERR_NOFILE), not the expected loader code.

Cause: the smoke script put the Zork volume (`win1-r11h-zork.img`) on the second drive. That volume
does not contain the ten malformed images (`BADMAG`, `BADENT`, `ENTOFF`, `BADSYM`, `BIGTXT`,
`RELOC`, `SHORT`, `TRUNC`, `TRAIL`, `EMPTYX`). The matrix runs NEG on
`win1-r11h-neg.img`, which does. So the QEMU run had missing inputs.

Fix: `build/fat16/win1-r11h-smoke.img` is the Zork volume plus the ten malformed images. Its root
directory is a strict superset of the Zork volume (57 entries plus the 10 added). The smoke script now
uses it.

Confirmation on QEMU VGA with the smoke volume (monitor capture, `evidence-qemu-neg/qemu-vga-neg-screen-capture.txt`):
`NEG: churn 40 cycles, stale ids refused 780`, `NEG: checks 848, failures 0`, `NEG: PASS`. The same
capture shows MULTI PASS x6.

Note on the xdotool smoke run: in this session the xdotool keystrokes did not reach the QEMU window in
the rerun (screenshots show the idle prompt). The first smoke run's screenshots were produced with
keystrokes that did arrive. The monitor-driven capture is the reliable path and is the one used for the
NEG result above.

## 3. Multiprocess, Phoon, Zork (matrix)

- MULTI: PASS on both emulators, 3 rounds each.
- Zork I (`ZORK1_Z3`, private test volume, not redistributed): 24 score-line blocks on both emulators, 3 rounds each, no internal errors.
- Phoon (pinned clock, 24 blocks): Tiny386 and i386VM agree on 24/24. One difference against `logs/r11f/phoon-battery-target.txt` at "7 Feb 2106 06:28:14": the R11F target exits 1, R11H exits 0. Unresolved; see section 6.

## 4. FAT16 volume hashes

Release volume `build/fat16/win1-r11h.img` (sha256 672806ad2c2ca337c45368dc6a16b71f2a8e61886a011bfbbf49d4cba5f1477b):
unchanged before and after the regression and the matrix (`VOLUMES UNCHANGED`).
Smoke volume `win1-r11h-smoke.img` (sha256 0067ec931eade375f2187b724f9bf0dafb22e2113653ae7d13657454606a6a52): unchanged by the QEMU run.

## 5. Regression (R11F seq-regression and seq-pinned, win1-r11h)

`scripts/r11h-regression.sh`, both emulators, none and pin modes. Expected differences, documented:
six new DIR entries (INFOCOM plus five R11H programs), heap shift of 2,608 bytes, FPMULT timing.
Result: `VOLUME UNCHANGED`, `REGRESSION DONE`, rc=0.

## 6. Open items

1. Phoon "7 Feb 2106 06:28:14": R11F log refuses, R11H pinned runs accept. Not explained.
2. i386VM initial ESP (0x7FFFF0) not verified in source.
3. Not reached by guest tests (documented in docs/R11H-MULTITASKING-TESTS.md): APP_ERR_ADDROCC,
   APP_ERR_NOJOB, region exhaustion, copy-failure rollback.
4. Shell START/PS: deferred, not implemented in R11H.
5. Clean-extraction rebuild: see packaging notes below.

## 7. Clean checkout and reproducible rebuild

Checked from `git archive` of the final commit into an empty directory, with the historical
toolchain tree (i386-gcc, as, i386-ld) and `make -C sdk/libc`, `sdk/libm`, `sdk/compiler`,
`sdk/lib` before the programs:

- Programs (`scripts/r11h-build.sh`): MULTI, MULTIA, MULTIB, COLLIDE and NEG are byte-identical to
  the working-tree build (sha256 compared per file).
- Building the clean tree first failed at link time because `sdk/ld/slot1.ld` used a multi-line
  comment that the historical i386-ld rejects. Fixed (single-line comment, commit a5caa1e).
- PC kernel (`make pc PCD=build/pc-r11h I386VM=<tree>`): the clean build does not match the
  working-tree `kernel.aout` / `kernel.raw`. The working-tree images were built with options that are
  not captured in the scripts, so the difference is not explained. NOT reproduced.
- i386VM images: not rebuilt in the clean tree. NOT reproduced.

## 8. Evidence

Under `evidence-qemu-neg/` (copied from scratchpad r11h/): the VGA capture, the old capture, the smoke
run log and the first run's NEG screen. Matrix and regression logs remain in scratchpad r11h/final and
r11h/regression.
