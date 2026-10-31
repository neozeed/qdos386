# QDOS/386 R5 - runtime report

R5 loads and runs two independently linked native i386 a.out programs as
ordinary QDOS/386 jobs. The loader is in the kernel and validates every image
before anything is written. Both targets use the same transfer area and the
same loader.

## 1. Results

| Target | Runs | Checks ok | FAIL | Result |
|---|---|---|---|---|
| i386VM (synthetic timer) | 3 | 103 each | 0 | `QDOS/386 R4 CHANNEL PASS`, `QDOS/386 R5 A.OUT EXEC PASS` |
| Tiny386 PC (known-good BIOS) | 3 | 103 each | 0 | both PASS lines |
| i386VM, timer disabled (negative) | 1 | - | - | no PASS; `instruction limit exceeded` |

Each run has one n/a check, as in R4: on i386VM the PIC in-service check (no PIC
on that target), and on the PC the device-ticks check (not applicable to that
target's timer wiring).

The guest prints `QDOS/386 R5 A.OUT EXEC PASS` only when every check, including
the application checks, has passed. The application checks confirm that the
applications actually executed: exit statuses 7 and 42 are recorded by
`QT_EXIT` and read back through `QT_JOB_EXIT`.

Logs: `logs/i386vm-run{1,2,3}.log`, `logs/pc-run{1,2,3}.txt`,
`logs/i386vm-notimer.log`. The R2–R4 logs remain in `logs/r5-phase1/` and
`logs/known-good-bios/` for comparison.

## 2. What was checked

Check breakdown per run (103 ok, 1 n/a):

| Group | ok | Notes |
|---|---|---|
| R2 scheduler, R3 heap, PIC and R4 channel checks | 54 | the R2–R4 checks, unchanged in meaning |
| R5 exit trap (`QT_EXIT`, `QT_JOB_EXIT`) | 5 | recorded status, no status, running, never created, slot reuse |
| R5 transfer area | 4 | directory valid, BSS below the area, two images present, area unchanged at the end |
| R5 malformed images | 22 | truncation, magic, relocation, entry, layout, format, string table, `_start`, directory, slots and heap unchanged |
| R5 loader and slots | 7 | NOJOB, NOMEM, refused load leaves slot 1 alone, placement, text/data match, SLOTBUSY |
| R5 applications | 11 | exit statuses, output, preemption, channel closure, stack faults, slot release, repeated load, heap restored |

## 3. Development finding (fixed)

The first live run printed both PASS lines before the application phase had
finished. The report was released by `r4_phase_done`, which the channel phase set
before `app_phase()` ran. The gate now uses a flag that only the application phase
sets, as its last step. The results in section 1 come from the fixed build only.

## 4. Transfer and startup evidence

- The i386VM image has the transfer area at `0x280000`, zero-padded from the
  kernel. The directory is read at boot and found valid, so the file load placed it
  correctly and BSS clearing did not destroy it.
- The PC image has the transfer sectors after the kernel. The boot sector reads
  both with one INT 13h call into the real-mode buffer at `0x10000`. The copy to
  `0x280000` happens in protected mode, and the directory is found valid.
- The "transfer area unchanged" check compares a checksum of the area at the start
  and end of the application phase, and passes on both targets.

## 5. Tests and images

- Malformed images are mutated copies of `hello` in a heap block. The validator is
  pure, and the check "malformed images left both slots and the heap unchanged"
  compares slot checksums and heap free bytes before and after.
- The heap-exhaustion test fills the heap with blocks until nothing fits, then
  refuses a load. Slot 1 is unchanged and unowned afterwards.
- The job-exhaustion test holds two job slots, then refuses a load into a free
  application slot.

## 6. Limits and known gaps

- The timer-disabled negative stops in the R2 timeout stage, as the R4 negative
  did. It therefore does not isolate the R5 preemption check. Its result is no PASS.
- The i386VM image is 2.6 MB because the transfer area is at `0x280000` and the
  kernel is small. The padding is zeros.
- The loader accepts only `0407` with no relocation records. Other magics and
  relocation are refused, and no other link configuration was tested.
- The `QDOS/386 R4 CHANNEL PASS` line is still printed with the R5 line, because
  the R2–R4 checks are part of the same run.
- The channel and the application console write through the same unbuffered
  driver, so output interleaves with kernel diagnostics (see the log).
- Build prints the two pre-existing `volatile` pointer warnings from `kernel/jobs.c`.
  They are unchanged from R4.

## 7. External dependencies (not redistributed)

| Item | SHA256 |
|---|---|
| Tiny386 `tiny386_headless` | `030cda746a37ab8e81bb21725232dfa70ca413ce448258e3f7e2c31791cce8a7` |
| i386VM `i386vm` | `baa683711ecfe674a0e3b226e166db3160aec59ab922b2ea264b146590082a6c` |
| SeaBIOS `bios.bin` (known-good, 131072 bytes) | `7bd968419c81fcf8262c5c1182cef43bb0d5eb70f375f94567ae71fd64a15e9e` |
| SeaBIOS `vgabios.bin` (known-good, 39424 bytes) | `f5aedb52cde5c88d85e5b304a4cc48459c231aef42b9950ccc82bd321d9c127e` |
| `i386-gcc` driver | `3567f3beab83e17d7f7b5ff5844ee1dd85fd452b84128bdc0a7d46bb103d7904` |
| `cc1` (GCC 1.40) | `1dd52eeb51191b0f1e6f92fcc9ed0be9e5fb80a97f28e41ddddb479f7399effb` |
| `cccp` | `a5d10c5c176cad35dae155194816e9633b923e8d3fafab100bdb6050dcf23cb5` |
| `as` (GAS 1.38) | `47369dd54851448080ce6836304829ea6ae98fbcb8efa944386c0ec0239d1009` |
| `i386-ld` (binutils 1.94-beta) | `dfdb396514559958d9bd1ed651337abded3565f6665d8bad05dcaeec92d07c20` |

The toolchain binaries are those of the i386VM harness tree. They were not
modified.
