# QDOS/386 R8 — Phase 0 audit

Scope: the frozen R7 tree (`qdos386-R7.tar.gz`, commit 8949f34 on branch r8),
audited from source and from the R7 logs. Items marked **measured** come from
a run in `logs/r7/phase5/` or from the boot banner. Items marked **source**
come from reading the file named.

## 0. Freeze and reproduction

- Fresh extraction: the R7 package was extracted into an empty directory and
  all 271 manifest entries verified (`sha256sum -c SHA256SUMS`).
- Clean rebuild: `git archive` of the R7 tree, `make apps fat16 i386vm pc
  i386vm-shell pc-shell i386vm-shell-notimer` produced the six images
  byte-identically to the working tree (matrix hashes in
  `logs/r7/phase5/images.sha256`).
- Regression: R2–R7 suite on both targets, three rounds, 177 CHECK lines each,
  0 FAIL (`logs/r7/phase5/`).

## 1. Memory map (measured and source)

| range | i386VM | Tiny386 PC | owner / use |
|-------|--------|------------|-------------|
| `0x000000` | kernel image and BSS, to `0x12DE0` (measured: arena starts `77280`) | not used | kernel |
| `0x100000` | not used | kernel image and BSS, to about `0x1132A0` (measured: arena starts `1127072`) | kernel (PC link address) |
| kernel heap | `0x012DE0..0x200000` (2,019,872 bytes, measured) | `0x1132A0..0x200000` (970,080 bytes, measured) | job stacks, channels, user blocks |
| slot 0 | `0x200000..0x23FFFF` (256 KiB) | same | shell |
| slot 1 | `0x240000..0x27FFFF` (256 KiB) | same | RUN children |
| transfer | `0x280000..0x2BFFFF` | same | R5 directory and images (test) |
| RAM disk | `0x2C0000..0x5BFFFF` (3 MiB, FAT16 image preloaded) | free RAM (the FAT16 disk is hdb, not RAM) | i386VM: volume; PC: unused |
| unused | `0x5C0000..0x5FFFFF` | `0x2C0000..0x5FFFFF` | **not allocated by any code** |
| floor | `0x600000` (`RAMDISK_FLOOR`, stacks and firmware start here) | same | firmware / boot |
| boot stack | grows down from `0x7FFFF0` | same (`platform/pc/entry.s`) | boot only |

Diagram (not to scale):

    0x000000 +-----------------------------+
             | kernel (i386VM)            |
    0x012DE0 +-----------------------------+
             | kernel heap (job stacks,   |
             | channels, user blocks)     |
    0x200000 +-----------------------------+  slot 0 (shell, 256 KiB)
    0x240000 +-----------------------------+  slot 1 (RUN children, 256 KiB)
    0x280000 +-----------------------------+  transfer area (256 KiB)
    0x2C0000 +-----------------------------+  RAM disk (i386VM, 3 MiB)
             | PC: unused                 |
    0x5C0000 +-----------------------------+  unallocated on both targets
    0x600000 +-----------------------------+  firmware / stacks floor
    0x7FFFF0   boot stack (grows down)

**Decision:** R8 keeps the two 256 KiB slots, the transfer area and the RAM
disk exactly where they are. The unused `0x5C0000..0x5FFFFF` range and the PC's
`0x2C0000..0x5FFFFF` are not used for a larger slot. Reasons: the i386VM RAM
disk and its end are fixed by `RAMDISK_SIZE`; the floor at `0x600000` is a
firmware and stack boundary (`docs/R3-FW-VALIDATION.md`) that was not
re-verified for R8; and a slot that moves would invalidate every linked
application's origin.

## 2. Loader (source: `kernel/aout.c`, `kernel/imgload.c`)

- `aout_check` (pure) validates: magic `0407`, relocation counts zero, entry
  equal to a slot base, text/data/BSS alignment to 16, text+data+BSS within a
  slot, symbol table size a multiple of 12, file length equal to header plus
  text, data, symbols and the string table (exact), `_start` present with the
  entry value.
- `app_load_bytes` calls `aout_check`, then claims the slot. The copy writes
  text, then data, then zeroes BSS, all under `irq_save`.
- **RUN** (`img_load_request`): reads the whole file into one kernel heap
  block (`mem_alloc(f.size)`), then calls `aout_check` on that block. The file
  size is limited by `IMG_MAX` = **8192**, a policy constant in
  `kernel/imgload.h`, checked before anything is read.
- The 8 KiB limit is therefore **policy, not a buffer size**. The heap could hold
  more, and the slot could hold 256 KiB. The limit comes from the single-block
  staging design, and R8 replaces that design (Phase 1).
- Test code has its own 8192-byte static buffer (`tests/test_shell.c`); it is
  test-only and is not part of the RUN path.
- Bounds: all reads use `unsigned char` pointers; sizes are compared without
  overflow (`len - off` after a `len <` check). No overflow found.

## 3. Job model (source: `kernel/jobs.c`, `kernel/jobs.h`)

- **Stack:** `job_alloc` allocates `JOB_STACK_BYTES` = **4096** from the heap,
  owned by the job id, with a four-word canary at its base. `stack_top` is the
  block end. Stack use is recorded in `stack_hwm` at reclaim. The stack size is
  not a parameter, so every application gets 4 KiB. A larger application with
  local arrays or recursion would overflow it silently into its heap block
  (canaries catch some of that at reclaim, not during the run). **R8 must make
  the stack size a parameter.**
- **Heap:** applications have no heap of their own. `QT_MEM_ALLOC` allocates
  from the kernel arena, owned by the calling job. Exit reclaims every block
  the job owns (`mem_reclaim_owner`). The arena is the only application heap:
  about 970 KB on the PC, 2 MB on i386VM.
- **Exit:** `QT_EXIT` records the status in `exit_rec[slot]` with the job id.
  `QT_JOB_EXIT` returns it while the record matches the id, and `JOB_ERR_NOSTATUS`
  if the job ended without it. Slot reuse changes the tag, so a stale id fails.
- **Reclaim:** `reclaim_dead` runs at the start of each trap. The slot is freed
  only after the frame is saved (`dead_mask`).
- **Entry:** `app_load_bytes` creates the job with `app_not_started` as a
  placeholder, sets `entry` to the image entry (`_start`), then sets
  `prio_inc = 2`, all under `irq_save`. `job_run` calls `entry` and then
  `job_exit`.
- **Traps:** interrupt gate, IF clear inside the trap. `QT_CH_READ` returns at
  most 256 bytes (R6 ABI). Any work done inside a trap runs with the scheduler
  stopped. **A RUN load of a large file is therefore one long uninterruptible
  section;** the R8 design bounds it and reports its size (Phase 1).

## 4. Startup and arguments (source: `apps/common/app_start.s`, `qtrap.s`)

- `_start` pushes EBP, calls `_main` with no arguments, passes the result to
  `QT_EXIT` (0x18) and returns. It does not clear BSS (the loader does).
- Applications are `int main(void)`. **No argv exists today.** The shell
  passes only a name (`QT_IMG_LOAD` takes a name pointer).
- A callee written `main(void)` in cdecl ignores extra stack arguments, so
  `_start` can push `argc` and `argv` without breaking R7 binaries.

## 5. Channels and files (source: `kernel/win1.c`, `kernel/channel.c`, `kernel/fat16.c`)

- `WIN1_` file channels: `QT_CH_READ` (≤256 bytes), `QT_CH_SEEK` (position,
  refused past the end). Reads are non-blocking; a file read is synchronous
  with the disk (`disk_read_sector`).
- Channel names: 16 bytes including NUL (`CHAN_NAME_MAX`). `WIN1_` leaves 10
  characters for the file name.
- FAT16 chains are validated at open (`fat16_open_root`). Size is taken from
  the directory entry. The volume is read only.
- A streamed load needs no new FAT16 entry point: `fat16_read` with a cursor
  and `fat16_seek` are enough.

## 6. Shell (source: `apps/shell/shell.c`)

- Line: 79 printable characters (`SH_LINE_MAX` 80). `RUN` name at most 10.
- `RUN` maps loader errors to fixed messages. Its only size message is for
  `APP_ERR_TOOBIG`.
- The shell waits for each child (`wait_job`) and then for slot reclaim
  (`QT_JOB_RECLAIMED`). One child at a time.

## 7. Volumes and tests

- The R7 volume (`build/fat16/win1.img`) is asserted in detail by the R7
  regression (`tests/test_dir.c`: exactly ten regular files, their order and
  sizes, `BIG.AOU` 8200 bytes). Adding files would change those assertions.
  **R8 keeps the R7 volume for the R2–R7 regression and builds a separate
  R8 volume** (`build/fat16/win1-r8.img`) for the R8 shell images and the R8
  applications. The R7 volume SHA256 does not change in R8.
- The R7 shell session (`tests/interactive/session.py`) expects `RUN BIG.AOU`
  to report `too large`. That is R7 behaviour and it stays with the R7 image.
  R8 has its own session with new expectations.

## 8. Risks found and how R8 addresses them

| # | finding | R8 response |
|---|---------|-------------|
| 1 | 8 KiB is a staging-buffer policy, not a slot limit | Stream into the slot (Phase 1); the cap becomes a documented limit on text+data+BSS |
| 2 | Job stack fixed at 4 KiB | Stack size becomes a parameter; applications get a documented size |
| 3 | No argv | Argument block owned by the job; `_start` calls `main(argc, argv)` |
| 4 | RUN load is one trap with IF clear | Bound the work; report the sector count; the load is never run from the timer |
| 5 | Shell and volume tests tied to R7 listing | Separate R8 volume; R7 image unchanged |
| 6 | Heap arena differs between targets (970 KB vs 2 MB) | Large test applications use slot space; heap test sizes use the smaller PC arena |
| 7 | `aout_check` requires the whole image | Split into header/symbol validation (no image needed) and a copy step |

## 9. Items not verified in Phase 0

- Wall-clock time of a large RUN on Tiny386 (no guest clock). R8 reports the
  number of sector reads instead.
- Behaviour of SeaBIOS above `0x600000` after the R8 boot path (the boot path is
  unchanged in R8; the R3 validation still applies).
- The GCC 2.95.3 bundled libc. Not supplied. The attached archive is a
  68000-targeted C68 library (see `docs/R8-SDK-INVENTORY.md`, Phase 4).
