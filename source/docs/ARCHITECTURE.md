# QDOS/386 architecture (canonical R18/R20 PC baseline)

This is the map for the supported PC kernels (R18 and R20, headless and VGA; see `docs/CONFIGURATION.md` and
`docs/BUILD.md`). Each subsystem's contract is in its header or source file header; the table below names them and
`docs/SOURCE-INVENTORY.csv` lists every file with its purpose and its history. Design rationale stays in the dated
documents linked from each section.

## Layers

| layer | files | role |
|---|---|---|
| platform (PC) | `platform/pc/*.c`, `platform/pc/*.s` | boot hand-over, PIC and PIT, the IDE disk (polled PIO, `disk.c`), VGA and the debug port 0x402, keyboard, RTC. |
| trap and context | `kernel/context.s`, `kernel/kctx.s`, `kernel/kctx17.s` | save and restore of a job's stack frame; the trap entry used by the service traps. |
| jobs and scheduler | `kernel/jobs.c|h`, `kernel/taskmgr.c`, `kernel/jobctl.c`, `kernel/smp*.c` | the job table, the ring-0 and CPL3 jobs, time-slicing, foreground-first placement on R16B and later. |
| memory | `kernel/memory.c`, `kernel/memmap.c`, `kernel/frames.c`, `kernel/pheap.c`, `kernel/paging.c`, `kernel/vas.c`, `kernel/proc.c` | the kernel heap, the firmware memory map (R17), the frame pool, paging and per-process address spaces. |
| applications | `kernel/aout.c`, `kernel/imgload.c`, `kernel/appmap.h`, `kernel/env.c` | loading of a.out images into the application slots, the environment block. |
| channels and files | `kernel/channel.c`, `kernel/win1.c`, `kernel/fat16.c`, `kernel/pathres.c`, `kernel/sync.c` | the channel table (`QT_CH_*`), the `WIN1_` FAT16 driver, the FAT16 and VFAT code, storage sync. |
| storage worker and cache (R20) | `kernel/r20w.c|h` | the ring-0 disk worker, its request table and the bounded sector cache. See `docs/STORAGE.md`. |
| consoles and display | `kernel/vcon.c|h`, `kernel/surface.c|h`, `kernel/gsurf.c|h`, `kernel/font*.c|h`, `kernel/kev.c|h`, `kernel/kbd.c|h` | virtual consoles, sessions and surfaces, fonts, key events. See `docs/DISPLAY.md`. |
| clock and timers | `kernel/clock.c|h`, `kernel/clock_boot.c`, `kernel/rtc.c|h`, `kernel/irq.c|h` | the kernel clock, the CMOS RTC at boot, the interrupt classification of the 8259 pair. |
| shell and boot | `kernel/shellboot.c`, `kernel/mbr.c`, `kernel/win1.c`, `kernel/main.c` | boot order, the built-in shell job, the first mount. |

## Boot order (PC)

1. The platform boot sector loads the kernel and the transfer area. On R17 and later the QBOOT3 sector also stores the
   E820 map at `0x504` (`kernel/memmap.h`).
2. `kernel/main.c` and `platform/pc/platform.c` bring up the platform: the debug port, the PIC and PIT, the VGA console when
   built with `QDOS_CON_VGA`.
3. Memory: the memory map is captured and validated (R17), the frame pool and paging are set up, then the kernel heap.
4. On R20, `r20c_init()` sizes and allocates the cache before the mount, because the mount reads through the cache.
   `r20w_start()` creates the worker job after the application processors are up (`docs/A16-CACHE-DESIGN.md`).
5. `win1_mount()` mounts the FAT16 volume; a mount or media change calls `r20c_invalidate_all()`.
6. The shell job starts; `STARTUP.CMD` on the volume runs before the first prompt.

## Process and job model

- A *job* is a kernel schedulable unit. A *process* is a job with an application image and a private address space when
  paging is on. Job ids are tagged: the slot index and a tag that changes when the slot is reused.
- Application code runs at CPL3 on R16 and later when marked or flagged. Ring-0 jobs (the storage worker, the AP idle jobs)
  have their own supervisor stacks.
- Channels, virtual consoles and surfaces belong to the job that opened them, or to the session it runs on. Reclamation of a
  job releases its channels, heap blocks and cache reservations (`r20w_job_gone`).

## Where to read more

- Scheduler and CPL3 policy: `docs/R16-ARCHITECTURE.md`, `docs/R16B-SCHEDULER-POLICY.md`, `docs/R16-CPL3-ABI.md`.
- Memory map and frames: `docs/APOLLO13-MEMORY-MAP.md`, `docs/R13-MEMORY-BUDGET.md`.
- Display: `docs/R13-DISPLAY-ABI.md`, `docs/R13A-GRAPHICS-ABI.md`, `docs/APOLLO14-DISPLAY-ABI.md`.
- Storage sync: `docs/R12C-SYNC-DURABILITY.md`. Cache: `docs/A16-CACHE-DESIGN.md`, `docs/A16-CACHE-RESULTS.md`.
- Context switch history: `docs/KCTX-FIX2-IMPLEMENTATION.md`.
