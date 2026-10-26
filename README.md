# QDOS/386 R3 - native kernel with a managed heap

A native 32-bit i386 kernel, freestanding, flat address space, CPL 0, paging
off. R3 adds a bounded, non-moving kernel heap, heap-backed job stacks, and
memory service traps. The R2 job scheduler, traps and timer behaviour are
kept, and they still pass unchanged.

Targets: the i386VM harness (synthetic timer) and the full Tiny386 PC
(SeaBIOS, PIT/PIC). Both run the same kernel sources.

## Status

Verified in the guest on both targets, three runs each:

- i386VM: `QDOS/386 R3 MEMORY PASS`, 33 checks ok.
- Tiny386 PC, known-good BIOS: `QDOS/386 R3 MEMORY PASS`, 32 checks ok, 1 n/a.
- i386VM with the timer disabled: no PASS (instruction limit exceeded).

Firmware validation (`docs/R3-FW-VALIDATION.md`): the known-good BIOS
reproduces R3, and R3 also passes without a VGA option ROM. The memory map
and interrupt paths were reviewed against the firmware source and the final
E820 table. The R3 kernel source is unchanged from `qdos386-R3.tar.gz`.
The earlier local SeaBIOS build is kept for comparison only (`external/`).

Details, limits and the exact checks are in `docs/R3-RUNTIME-REPORT.md`.

## Layout

```
kernel/
  memory.h, memory.c     native heap: mem_init, mem_alloc, mem_free,
                         mem_reclaim_owner, mem_stats, mem_validate
  jobs.h, jobs.c         job table, scheduler, traps (R2 + memory traps)
  context.s              IRQ/trap stubs, yield, resume, IF save/restore
  descriptors.c          GDT, IDT (IRQ0, spurious IRQ7, trap 0x30)
  main.c                 boot, heap-only tests, R2 sequence, job phase, report
tests/
  test_jobs.c/h          R2 test jobs TEST-A, TEST-B, TEST-C
  test_memory.c/h        R3 heap tests and memory job phase
platform/
  i386vm/                console and synthetic timer, halt
  pc/                    Tiny386 console, PIC/PIT, halt, boot sector, image
docs/
  JS-ROM-MEMORY-MAP.md   Phase 0 audit of the JS ROM memory services
  R3-MEMORY-DESIGN.md    arena, blocks, ownership, ISR rules, stacks
  R3-MEMORY-ABI.md       native trap calling convention and codes
  R3-RUNTIME-REPORT.md   commands, environment, results, limitations
  JS-ROM-KERNEL-MAP.md   R2 scheduler map (stacks are now heap-backed; see R3 design)
  TINY386-COMPATIBILITY.md, R1-RUNTIME-REPORT.md   from R2
  R3-FW-VALIDATION.md    known-good firmware run, memory map, interrupt review
  R4-CHANNEL-DESIGN.md   R4 channel manager design note (not implemented)
scripts/
  run-all.sh             build both targets and run the matrix
logs/
  known-good-bios/       R3 matrix with the known-good BIOS, and the no-VGA run
  seabios-local-build/   R3 matrix with the previous local SeaBIOS build
  r2-baseline/           R2 regression logs for comparison
  (scripts/run-all.sh writes i386vm-run*.log, pc-run*.txt, i386vm-notimer.log here)
external/
  seabios-local-changes.diff, seabios-untracked.txt
patches/
  i386vm-synthetic-timer.patch   i386VM synthetic timer (applies to the harness)
```

## Build and run

External inputs are not included. Set their locations:

- `I386VM`: the i386VM harness tree (`bin/i386-gcc`, `bin/i386-ld`, `bin/i386vm`).
- `TINY386`: the `tiny386_headless` binary built from the Tiny386 tree.
- `SEABIOS`: a directory with `bios.bin` and `vgabios.bin`. The validated baseline is the known-good pair in `docs/R3-FW-VALIDATION.md` §2; the earlier local SeaBIOS build also works.

```
I386VM=... TINY386=... SEABIOS=... scripts/run-all.sh
```

Or build only: `make i386vm pc I386VM=... TINY386=... SEABIOS=...`.

Outputs:

- `build/i386vm/qdos386-r3.bin`: flat image for the i386VM harness.
- `build/pc/r3pc.img`: Tiny386 hard-disk image (SeaBIOS boots the boot sector).
- `build/i386vm/qdos386-r3-notimer.bin`: negative-test image.

## Memory services (summary)

Trap `int 0x30`, EAX = function, EBX/ECX/EDX = arguments, EAX = result.

| Code | Service |
|---|---|
| `0x10` | allocate: EBX = size → pointer, owned by the caller |
| `0x11` | free: EBX = pointer → 0 or error |
| `0x12` | free bytes in the heap |
| `0x13` | validate the heap |

Full definitions, error codes and deviations from the JS ROM are in
`docs/R3-MEMORY-ABI.md`. These services are native i386 extensions. They are
not the 68000 trap ABI, and they are not compatible with QDOS.

## Not in R3

- JS ROM BASIC relocation and transient-area growth (deferred).
- Channels, filesystem, ELF loading, SuperBASIC.
- Dynamic job-table size (fixed at 4).
- Host-side allocator tests. None were written, and none are claimed.
