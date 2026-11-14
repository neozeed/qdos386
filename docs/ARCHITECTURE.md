# Current QDOS/386 architecture

> Preserved R3 baseline document. For current R4 resource changes and profile status, see [R4 configuration](R4-CONFIGURATION.md), [ABI review](R4-ABI-REVIEW.md) and [known limitations](R4-KNOWN-LIMITATIONS.md). The R3 text below is retained for provenance.

The current production path is root `make production` → `source/Makefile` → `source/mk/current.mk`. `source/` is authoritative; root kernel/platform duplicates and older release recipes are historical material. The explicit legacy diagnostic composition is a different boot/profile, described in BUILD and CONFIGURATION. The accepted R2 tag remains reproducible and is not rewritten.

## Boot and execution

QBOOT3 loads the kernel and transfer area, records firmware E820 data, and enters protected mode. `platform/pc/entry.s` sets the boot stack, clears BSS and calls main. Production `kernel/main.c` only calls shell_boot. `shellboot.c` captures E820, installs descriptors/input, initializes scheduler/FPU/heap, discovers CPUs, enables paging, reserves the page-backed cache, mounts WIN1, loads SHELL.AOU, creates BSP idle, installs Task Manager/system polling, initializes fonts/clock/serial, starts timers/APs, and finally starts IOW0. Cache reservation precedes ordinary process directories and AP execution.

CPU0 owns PIT timekeeping, device interrupts, poll lists, foreground presentation and related system work. AP LAPIC timers schedule their own CPU without incrementing the shared wall-clock/PIT tick stream. AP startup/discovery/accounting is in smp17.c/smp_s17.s; active context entry is kctx17.s. Eight is the fixed context bound, not a promise that all firmware-discovered processors start.

Native protected images run at CPL3 with private image/heap/stack mappings and a shared supervisor kernel. The loader still understands existing legacy formats; not every accepted image is automatically a protection claim. Interrupt/trap entry switches to the job's kernel stack when needed, takes the BKL, executes C handlers, selects the proper CR3/frame, and releases the lock before user return. See ABI and LOCKING.

## Ownership and interfaces

| Owner | State and interface |
|---|---|
| jobs.c / smp17.c | Tagged jobs, status/priority/affinity, per-CPU current/idle state, deferred reclamation and foreground policy |
| proc.c / frames.c / vas.c / pheap.c | Address spaces, frame membership/ownership, process heap/stack and kernel-entry stacks |
| channel.c / win1.c / fat16.c | Owned channels, file cursors, FAT/VFAT metadata and synchronous writes |
| r20w.c / PC disk.c | Worker tickets/cache lifetimes and serialized PIO; worker does not own process buffers |
| vcon.c / surface.c / gsurf.c / presenter | Sessions, focus, surfaces, fonts, pixels/palette and foreground-only hardware presentation |
| i8042.c / kbd.c / kev.c | IRQ/scancode translation, hotkeys, cooked input and per-session key events |
| taskmgr.c / sync.c | SYSTEM polling, task overlay and periodic synchronization; not a separate replacement scheduler |

All capacities, trap encodings and ownership rules are retained. Kernel diagnostics such as FPU checks, frame accounting and cache dumps remain production services; the historical R2–R8 test harness is not linked into production. The diagnostic kernel retains those tests and its original single-BSP/nonpaged profile, with an isolated bridge for trusted kernel-test service buffers. It does not weaken production pointer checks.

The linked-section map and source inventory are evidence, not a new kernel-object model. No new syscall, DMA/IRQ design, display mode, process-count expansion or application rewrite is included. Known limitations are in REMAINING-DEBT.
