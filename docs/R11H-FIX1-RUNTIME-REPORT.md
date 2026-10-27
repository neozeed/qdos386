# R11H-FIX1 runtime report

Base: frozen R11H source `a15bd80` (shipped `qdos386-R11H.tar.gz`, SHA256 of the release volume
`672806ad…477b` and PC shell image `8f22b0b2…2307` match the shipped `SHA256SUMS`).
Branch `r11h-fix1`. Final source commit: see `PACKAGE.md`.

Evidence is labelled. **Scripted** means keys were injected by the harness (Tiny386 serial
harness, i386VM stdin, or QEMU monitor `sendkey`); no human operated any session. Logs are in
`logs/fix1/`.

## 1. Emulators, firmware and toolchain (hashes, first 16 hex digits of sha256)

| item | identity |
|---|---|
| Tiny386 emulator of record | `tiny386-shipped-fb63b405` (`fb63b4057e3a62f5`), used for every Tiny386 run |
| Tiny386 other candidates (not used) | `36e29bb17a0844b3`, `d5da453542fe10d5` |
| i386VM harness | `i386vm-r9` (`a4724f79b26a716e`), built from `patches/` with `scripts/r11h-build-harness.sh` |
| i386VM input-status only | `i386vm-input-only` (`2d2f8beba177592e`) |
| firmware (PC runs) | `bios.bin` `7bd968419c81fcf8`, `vgabios.bin` `f5aedb52cde5c88d` (`build/pc-r11f-none-run/`) |
| QEMU VGA firmware | system SeaBIOS `/usr/share/seabios/bios.bin` `4d597f68e06a0e28` (see caveats) |
| historical i386-gcc | `3567f3beab83e17d`; as `47369dd548514480`; i386-ld `dfdb396514559958`; cc1 `1dd52eeb51191b0f`; cccp `a5d10c5c176cad35` |

Tiny386 identity: the MULTI and NEG transcripts cannot tell the three Tiny386 candidates apart
(identical deterministic lines, timer lines differ). The binary of record is recorded here by hash
only; the emulator is not in the package.

## 2. Verification matrix (`scripts/r11h-matrix.sh`, scripted)

Volumes before and after the matrix: `win1-r11h.img` `672806ad…477b`, `win1-r11h-neg.img`
`5af61837…ed`. Both unchanged (`logs/fix1/volumes-before.txt`, `volumes-after.txt`, `VOLUMES UNCHANGED`).

| run | Tiny386 (`fb63b405`) | i386VM (`i386vm-r9`) |
|---|---|---|
| MULTI round 1–3 | PASS, PASS, PASS | PASS, PASS, PASS |
| NEG round 1–3 | 848 checks, 0 failures, PASS ×3 | 848 checks, 0 failures, PASS ×3 |
| Phoon battery (release volume, 1 round, unpinned) | 24 runs, 22 match the R11F battery reference; two differ, explained below | same 24 runs and same two differences |
| Phoon 2106 fixture (`tests/fix1/seq-phoon-2106.txt`) | 13 accepted, 14 accepted, 15 refused, 2100 accepted: `PHOON 2106 CHECK: PASS` | same: `PHOON 2106 CHECK: PASS` |
| Zork ×3 | **not run**: private story volume not in this worktree (`ZORK: UNAVAILABLE`) | not run |

Phoon battery differences against `logs/r11f/phoon-battery-target.txt` (banner `QDOS/386 R7`, a stale
reference: its kernel predates the R11F time ABI):

- `7 Feb 2106 06:28:14`: reference refuses (exit 1), FIX1 accepts (exit 0). This is the Phase 3 boundary
  result; the supported range includes 06:28:14 (`docs/R11H-FIX1-PHOON-BOUNDARY.md`).
- `12:00:00 PM`: reference accepts (exit 0), FIX1 refuses (exit 1). This depends on the clock mode, not on
  the Phoon binary (both sides run the same `PHOON.AOU`, `df2a4f1e…`, recorded in the R11F artefact
  list). Measured on i386VM with the R11H kernel: unpinned refuses (`illegal date/time`), pinned
  (`-DQDOS_CLOCK_PINNED`) accepts. The R11F unpinned reference `logs/r11f/phoon-none-i386.txt` also
  refuses it, and the R11F pinned run accepts it.

Note on the first matrix Phoon run. `seq-phoon.txt` does not contain `06:28:13`, and the checker
reported `missing` for that case. This was a test-sequence gap, not a Phoon result. The fixture
sequence was then run on its own and passes on both emulators.

NEG 848 checks on both emulators includes the 780 churn stale-id refusals and the ten malformed-image
loader refusals (-20..-36 family), the same numbers as R11H.

## 3. Fault-injected rollback (Phase 4)

Test-only build: `make pc PCD=build/pc-fault PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE -DQDOS_TEST_FAULT_LAUNCH=4 -DQDOS_TEST_FAULT_READ=2"`. The
hook (`kernel/aout.c`, active only under `QDOS_TEST_FAULT_LAUNCH`) fails the second read of the fourth
image load with `FAT_ERR_IO` (-40), after `job_alloc_sized` and `region_reserve_at` and before any
image is written. The failure path releases the region and cancels the job (`aout.c` lines 529–533).

Guest program `FAULT.AOU` (`sdk/examples/r11h/fault.c`), Tiny386, scripted, `docs/evidence/fix1-fault-tiny386-launch4.txt`:

- MULTIA live on profile 2 (id 65540)
- faulted MULTIB launch on profile 3 returns -40
- occupied job slots unchanged across the failed launch (4 before, 4 after)
- MULTIA still running (-16) after the failure
- profile 2 still owned: COLLIDE refused with -28
- retry of MULTIB on profile 3 starts (no leaked region) and exits 7
- MULTIA exits 3, unharmed
- `FAULT: checks 8, failures 0`, `FAULT: PASS`

The first run used launch index 3 and failed MULTIA instead. Two loads precede FAULT's children (boot
shell and FAULT itself), so the index is 4. The index is an empirical property of the boot path;
the test checks the outcome, so a wrong index would show as a failure, not a pass.

Production build: the hook is not compiled without the define. The production PC image built from the
same tree reproduces `8f22b0b2613d0554` (identical to the shipped `r11h-shell.img`).

FAULT ran on Tiny386 only. There is no i386VM fault build in this package.

## 4. Region and job paths not reached by the guest

- **APP_ERR_ADDROCC (-36)**: `region_reserve_at` returns BUSY only when a requested range overlaps an
  existing reservation. The three fixed profiles (slots 1–3 at 0x240000, 0x380000, 0x500000) and the
  kernel heap that ends at 0x200000 are disjoint, and ownership of a profile is checked first (SLOTBUSY).
  Guest code cannot produce an overlap. BUSY semantics are tested on the host (below).
- **APP_ERR_NOJOB (-29)**: at most four a.out jobs can be live from the guest side (boot shell,
  profile-1 launcher, one child on profile 2, one on profile 3) against `JOB_MAX` 6. Exit reclaim is
  exercised by 40 churn cycles with no NOJOB. The job-allocation failure branch is not executed by any
  test; it is verified by reading `aout.c`.
- **Region exhaustion, fragmentation, overflow refusal, stale handles, ownership**: host unit tests in
  `tests/host/r11h/region_test.c` (`5052 checks, 0 failures`, `sh tests/host/r11h/run.sh`). Guest
  coverage of stale ids and ownership: NEG churn and the wrong-origin checks.

## 5. QEMU VGA (scripted monitor capture)

`scripts/r11h-qemu-capture.py`, kernel `build/pc-r11h-vga/r9shell-vga.img` (`0560ff32…`, equal to shipped),
volume: `win1-r11h-neg.img` (story-free release set plus malformed images), used as the smoke volume.
The earlier smoke volume contained the private story and is excluded from this package.

- `run multi.aou`: MULTI transcript complete (`round 5` once), `MULTI: PASS` on screen.
- `run neg.aou`: `NEG: checks 848, failures 0`, `NEG: PASS`.
- After NEG, the console returns to the prompt (`exit status 0`), so shell recovery is confirmed.
- 1825 changed screens recorded (`logs/fix1-qemu/lines.txt`). Repeated PASS lines are the same output
  captured across scroll snapshots, not repeated runs.

Caveat: this capture ran with the system SeaBIOS, not the `bios.bin` used by the Tiny386 runs.

## 6. Regression (R11F/R11G sequences on the release volume, scripted)

`scripts/r11h-regression.sh`. The script does not diff; the diff was done against `logs/r11f/` here.

Tiny386 and i386VM, `none` and `pin` modes. Differences from the R11F logs, after removing timer lines:

- six added directory entries (INFOCOM and the five R11H programs), 47 → 53 files: expected
- free-heap figures shifted by 2,608 bytes in both `none` and `pin` runs (kernel growth): expected
- i386VM `pin`: HALT EIP moved (`0000c3e5` → `0000cc99`); kernel code address shift from the larger kernel
- Tiny386 FPMULT: `seen=` counters differ (timing); clean, exact, exit codes and x87 `bits` identical

Volume hash unchanged across the regression runs.

## 7. Clean-extraction identity (Phase 5.6)

`git archive` of the committed tree into an empty directory, then rebuilt with the historical tree
(`QDOS386_TOOLS` = `I386VM` = `x/i386vm/i386vm`, i386-gcc `3567f3beab83e17d`). Results against the
shipped `SHA256SUMS`:

| artifact | rebuilt | shipped |
|---|---|---|
| `build/pc-r11h/kernel.aout` | `5fed2619e0a0e8d1` | `5fed2619…` identical |
| `build/pc-r11h/r11h-shell.img` | `8f22b0b2613d0554` | identical |
| `build/pc-r11h-vga/r9shell-vga.img` | `0560ff320177cc33` | identical |
| `build/r11h/{MULTI,MULTIA,MULTIB,COLLIDE,NEG}.AOU` | equal to shipped | identical |
| `build/i386vm-r11h-rel/qdos386-r11h-rel.bin` | `c5462138ea630fa7` | identical |
| `build/i386vm-r11h-rel/kernel-raw.bin` | `70902c40450c3786` | identical |
| `build/i386vm-r11h-neg/qdos386-r11h-neg.bin` | `43ba3cc3e6963e24` | identical |
| `build/r11h-maps/i386-r9.aout` | `2aa5a9eaa263a824` | identical |
| `build/r11h-maps/i386-r8.aout` | `dd9bcb1f19dac711` | identical |

The i386VM images take a FAT16 volume as an input. The clean build used the shipped
`win1-r11h.img` and `win1-r11h-neg.img` copied in as data; they are not rebuilt in this step.

**Not reproduced:** the R11F application set. Rebuilt from clean sources with the rebuilt SDK
libraries, each R11F `.AOU` is 38 bytes longer than shipped (for example CTYPE.AOU: text 0x3dc0 → 0x3dd0,
entry 0x96c → 0x978). The R11H applications are unaffected. The cause is not established; the historical
SDK archive used for the shipped R11F objects was not captured. The shipped volume is what the matrix
runs, so the verified results do not depend on this; the rebuilt volume is not a release candidate.
See `docs/R11H-FIX1-BUILD-REPRO.md`.

## 8. Phase 0 provenance

Baseline, toolchain and emulator identities are in `docs/R11H-FIX1-BASELINE.md`. The Tiny386 binary
was not recorded in R11H; it is recorded here (section 1).

## 9. Open items and limits

1. R11F application bytes do not rebuild from current sources (section 7).
2. ADDROCC and NOJOB are not reached by any guest test (section 4).
3. Zork I and the private story volume were not run; the matrix skips them and says so.
4. FAULT ran on Tiny386 only.
5. The QEMU VGA capture used system SeaBIOS, not the package firmware.
6. Emulator binaries are not shipped; they are identified by hash only, and the i386VM harness is built
   from the `patches/` directory against the historical emulator source.
7. "Diagnostic" and "interactive" boot paths: every run in this package is scripted (keys from the
   harness or the QEMU monitor). No human-operated session was performed.
