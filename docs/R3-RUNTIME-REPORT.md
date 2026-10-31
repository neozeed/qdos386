# QDOS/386 R3 - Runtime Report

## 1. Status

| Target | Build | Runs | Result |
|---|---|---|---|
| i386VM harness (synthetic timer) | verified | 3 of 3 pass | `QDOS/386 R3 MEMORY PASS`, 33 checks ok, 0 fail |
| Tiny386 PC, known-good BIOS (primary, §10) | verified | 3 of 3 pass | `QDOS/386 R3 MEMORY PASS`, 32 checks ok, 1 n/a (device ticks), 0 fail |
| Tiny386 PC, known-good BIOS, no VGA option ROM (§10) | verified | 1 run | `QDOS/386 R3 MEMORY PASS`, 32 checks ok, 0 fail |
| Tiny386 PC, previous local SeaBIOS build (comparison, §2) | verified, build not pristine | 3 of 3 pass | `QDOS/386 R3 MEMORY PASS`, 32 checks ok, 1 n/a (device ticks), 0 fail |
| i386VM, repeated in the firmware-validation pass (no BIOS involved) | verified | 3 of 3 pass | `QDOS/386 R3 MEMORY PASS`, 33 checks ok, 0 fail |
| i386VM, timer disabled (negative) | verified | 1 run | no PASS; `instruction limit exceeded` |

The PASS line is printed only after every guest check succeeds. Every check
below is a guest-side assertion made inside the running kernel. No host-side
checks are part of this report. The `host-test` target from the plan was not
written, so no host-side allocator results exist, and none are claimed.

## 2. Environment

- Host: Linux 6.18.44-fc-v153, x86-64. Host gcc 13.3.0 is not used for the
  kernel.
- Toolchain: GCC 1.40, GAS 1.38, binutils 1.94-beta `i386-ld`, from the
  i386VM harness tree. `i386-gcc` drives both the compiler and the link. The
  i386VM tree's `SOURCE-HASHES.txt` was not re-verified in this run.
- i386VM emulator: `i386vm` (synthetic timer from `patches/i386vm-synthetic-timer.patch`).
  SHA256 `baa683711ecfe674a0e3b226e166db3160aec59ab922b2ea264b146590082a6c`.
- Tiny386: `tiny386_headless` built from the unmodified Tiny386 tree.
  SHA256 `030cda746a37ab8e81bb21725232dfa70ca413ce448258e3f7e2c31791cce8a7`.
- SeaBIOS: built from `https://github.com/coreboot/seabios.git`, commit
  `28aa46349b97ef0ebf742ae4fb9f9fad03578670`. **The build tree was not
  pristine.** Nine source files had local modifications (97 insertions, 10
  deletions) and one stray `floppy.c.orig` file existed. The build reports
  `28aa463-dirty-20261026_110318-vm`. The diff is in
  `external/seabios-local-changes.diff`, and the untracked file is listed in
  `external/seabios-untracked.txt`. The `.config` used for the build was not
  kept, so the exact configuration is not reproducible from this archive.
  The BIOS images therefore must be treated as **not upstream-verified**. The
  guest results depend only on the BIOS behaving as a PC BIOS, which the boot
  and timer results confirm but do not prove.
  - `bios.bin` SHA256 `f33ed9836dd628c960757d43d9cbeaa037647063a2068cd3aabb143bf96adf91`
  - `vgabios.bin` SHA256 `192cce7be2adb9582a8f9c610b9f43af1e3edbd48a4a75b5ed14d3d1d75fa5b9`
  - In the firmware-validation pass (§10) this local build is compared
    against the **known-good** images, which are now the primary PC firmware:
    `bios.bin` SHA256 `7bd968419c81fcf8262c5c1182cef43bb0d5eb70f375f94567ae71fd64a15e9e`,
    `vgabios.bin` SHA256 `f5aedb52cde5c88d85e5b304a4cc48459c231aef42b9950ccc82bd321d9c127e`.
    Both are supplied, both are dirty builds, and neither source tree is in this archive.

External binaries (SeaBIOS images and both emulator builds) are **not**
redistributed in this archive. The run script takes their locations from
environment variables.

## 3. Commands

Full matrix, three runs per target, then the negative run:

```
I386VM=<i386vm tree> TINY386=<path>/tiny386_headless SEABIOS=<dir with bios.bin, vgabios.bin> \
  scripts/run-all.sh
```

Individual steps (what the script runs):

```
make i386vm I386VM=... TINY386=... SEABIOS=...        # build/i386vm/qdos386-r3.bin
make pc     I386VM=... TINY386=... SEABIOS=...        # build/pc/r3pc.img
make build/pc/run.ini I386VM=... TINY386=... SEABIOS=...
<i386vm tree>/bin/i386vm build/i386vm/qdos386-r3.bin
(cd build/pc && <path>/tiny386_headless run.ini)
make notimer I386VM=...                               # build/i386vm/qdos386-r3-notimer.bin
<i386vm tree>/bin/i386vm build/i386vm/qdos386-r3-notimer.bin
```

`QDOS_NO_TIMER` is defined only for the negative build. It omits
`timer_start`.

## 4. Measured memory map

| | PC (Tiny386) | i386VM |
|---|---|---|
| Kernel origin | `0x100000` (linker) | `0x000000` (driver) |
| Boot stack top | `0x7FFFF0` | `0x7FFFF0` (`crt0`) |
| Arena start | `0x1064E0` (after BSS) | `0x006430` (after BSS) |
| Arena end | `0x600000` | `0x600000` |
| Arena size | 5,217,056 bytes | 6,265,808 bytes |
| Heap at end of run | 1 used block (IDLE stack, 4,112 bytes), 2 free blocks | same |

The bounds are the constants in `kernel/memory.h` (`MEM_ARENA_END`). The
design rationale is in `R3-MEMORY-DESIGN.md` §1.

## 5. Checks

Each line is printed by the IDLE job. All 33 (i386VM) and 32 (PC, plus one
`n/a`) are `ok` in the three runs of each target.

R2 regression checks (unchanged in meaning):

| Check | Source |
|---|---|
| chprio | priority change and read-back |
| timeout wake | 3-tick suspend, flag set by timer |
| reactivate | indefinite suspend, released by TEST-B |
| poll/sched lists | both lists ran |
| slot reuse | TEST-C reused TEST-A's slot |
| stale id | TEST-A's old id rejected |
| new id | TEST-C's id differs from TEST-A's |
| device ticks | i386VM device counter (PC: `n/a`) |
| TEST-A, TEST-B preempted | `preempts > 0` in every run |

R3 heap-only tests (plan items 1–6, run before any job exists):

| Check | Plan item |
|---|---|
| heap alignment, zero-fill, reuse | 1 |
| block size recorded as rounded size | 1 |
| non-overlapping blocks keep their canaries | 2 |
| adjacent free blocks coalesce (both orders) | 3 |
| exhaustion: clean NOMEM, exact fit, full release | 4 |
| zero size, overflow and oversize rejected | 4 |
| bad pointer, wrong owner and double free rejected | 6 |
| heap invariants after heap tests | 8 |

R3 job-phase tests (plan items 5, 6, 7, 8, 10):

| Check | Plan item |
|---|---|
| MEM-1 and MEM-2 preempted while allocating | 7 |
| MEM-1 and MEM-2 contents intact across preemption | 7, 8 |
| dynamic stacks: high-water mark recorded | 10 |
| job exit reclaims its heap blocks | 6 |
| reclaimed block: stale owner and pointer rejected | 6 |
| reused slot gets a new tag | 6 |
| stale job id cannot reactivate reused slot | 6 |
| stale owner cannot free a live block | 6 |
| owner frees its own block; suspended job resumed | 6, 7 |
| repeated create/exit/reuse: distinct tagged ids | 6 |
| repeated create/exit/reuse: heap restored | 6 |
| job creation fails cleanly under memory pressure | 5 |
| pressure released: heap restored | 5 |
| creation succeeds once memory is available | 5 |
| heap invariants hold after the job phase | 8 |
| no stack canary failure seen by the ISR | 8, 10 |
| heap restored to baseline at the end | 8 |

Plan item 2 (8-byte internal allocation path) is **not** tested. The 8-byte
path of `MM.ALLOC` is not exposed (`docs/JS-ROM-MEMORY-MAP.md` §4.3).

## 6. Observed numbers

Measured from the final three-run matrix (`logs/`):

| Target | Reclaims | Bytes reclaimed | Stack faults | MEM-1 high-water |
|---|---|---|---|---|
| i386VM | 10 | 42,208 | 0 | 288 bytes (run 1), see logs |
| PC | 10 | 42,208 | 0 | 308 bytes (run 1), see logs |

The high-water values differ because the PC run's timing differs. The
counts are fixed by the test script. The PC timer is host-driven, so the
number of ticks, the preemption counts, and the output interleaving vary from
run to run. The checks assert invariants, not those numbers, as the plan
requires.

## 7. Intentional output changes from R2

- Banner: `QDOS/386 R3`.
- Heap lines (`Heap: arena …`, `HEAP:`, `RECLAIM:`, `STACK:`) are new.
- Final line: `QDOS/386 R3 MEMORY PASS` replaces `QDOS/386 R1 MULTITASKING PASS`.
- New `CHECK` lines (R3 items above). R2 `CHECK` lines are unchanged.
- The negative build prints `TIMER: disabled (negative test build)` instead of
  `TIMER: IRQ0 active`.

Behavioural changes that are not output changes:

- `yield_asm` masks interrupts for the duration of a cooperative pass (R2 did not).
- Dead-job reclaim no longer runs in the timer ISR.
- Job stacks are heap-backed.
- `kstart` clears IF, and spurious IRQ7 vectors have a handler (R2 had neither).

## 8. Limitations, per target

Both targets:

- Pointer checks are best effort (`R3-MEMORY-DESIGN.md` §3.1).
- Stack canaries are checked at switch-out. An overflow is reported late.
- Stack size is fixed at 4 KiB. Job-table size is fixed at 4.
- The 8-byte allocation path is not exposed.
- No host-side allocator tests exist.

i386VM:

- The synthetic timer runs on the step count, so the run is deterministic.
- The timer-disabled negative run correctly fails to finish (`instruction limit
  exceeded`). That run confirms there is no false PASS. It does not prove that a
  timer-only failure is always reported as FAIL.

PC:

- PIT timing is host-driven. Ticks, preemption counts and output order vary.
  Invariants are asserted instead.
- The device tick counter is not exposed on the PC, so that check is `n/a`.
- The arena's upper bound (`0x600000`) is a fixed choice. Its relation to the
  firmware map is now established from the final E820 table and the SeaBIOS
  source (`docs/R3-FW-VALIDATION.md` §4). SeaBIOS reserves `0x7C0000..0x7FFFFF`
  only temporarily and returns it before boot, so the boot stack top is in RAM.
  The range `0x600000..0x7BFFFF` is RAM that R3 leaves unused.
- A timer-disabled PC run was not made, because the PC has no step-count
  timer and a spinning job would never return.
- `kstart` clears IF. Before the spurious-IRQ7 fix, a PIT tick left pending
  by SeaBIOS aborted the emulator (`docs/R3-MEMORY-DESIGN.md` §5.2). The
  fix is verified on this emulator build. Other 8259 implementations may
  behave differently.

## 9. Files

- `docs/JS-ROM-MEMORY-MAP.md` — Phase 0 audit, closed with evidence.
- `docs/R3-MEMORY-DESIGN.md` — arena, headers, coalescing, ownership, ISR rules.
- `docs/R3-MEMORY-ABI.md` — native trap calling convention and codes.
- `docs/R3-RUNTIME-REPORT.md` — this report.
- `logs/known-good-bios/` — the firmware-validation matrix (known-good BIOS), plus
  the no-VGA-ROM run (`pc-novga.txt`).
- `logs/seabios-local-build/` — the previous local SeaBIOS matrix, for comparison.
- `logs/i386vm-run{1,2,3}.log`, `logs/pc-run{1,2,3}.txt`, `logs/i386vm-notimer.log`
  are not in this package; `scripts/run-all.sh` writes them to `logs/`.
- `docs/R3-FW-VALIDATION.md` — firmware validation, memory map, interrupt review.
- `docs/R4-CHANNEL-DESIGN.md` — R4 design note (not implemented).
- `logs/r2-baseline/` — the R2 regression logs, kept for comparison.
- `external/seabios-local-changes.diff` — local SeaBIOS modifications at build time.
- `external/seabios-untracked.txt` — untracked SeaBIOS files at build time.
- `SHA256SUMS` — checksums of every file in the archive except itself.

## 10. Firmware validation (known-good Tiny386 BIOS)

The supplied known-good `bios.bin` and `vgabios.bin` were run against the
unchanged R3 kernel. Full evidence is in `docs/R3-FW-VALIDATION.md`.

- The known-good BIOS reproduces R3: 3 of 3 PC runs pass, 32 checks ok, 1 n/a,
  0 FAIL. Results match the earlier local-build matrix in heap, reclaim and
  stack-high-water values.
- With no valid VGA option ROM (zero-filled placeholder), R3 still passes
  (1 run, 32 checks ok). Port `0x402` output does not depend on the VGA BIOS.
- The i386VM matrix (3 runs) and the timer-off negative still pass or fail as
  before.
- Memory map: the final E820 table the guest sees has RAM at
  `0x100000..0x800000`. The SeaBIOS high table at `0x7C0000..0x7FFFFF` is
  reserved only during POST and returned before boot. The heap arena
  `0x1064E0..0x600000` does not overlap the kernel, BSS, boot stack or firmware
  areas.
- Interrupt review: `kstart` `cli`, yield masking and `irq_save`/`irq_restore`
  are correct for the paths exercised. The spurious IRQ7 handler on `0x27` is
  correct. On `0x2F` it omits the master EOI that a spurious slave IRQ7 would
  need. That path is unreachable here because the slave is fully masked, and
  it is left as an open item (`R3-FW-VALIDATION.md` §5.3).
- Freeze state: the R3 kernel source is unchanged. The original archive is
  unchanged. This package is a separate artifact, `qdos386-R3-FW1.tar.gz`.

