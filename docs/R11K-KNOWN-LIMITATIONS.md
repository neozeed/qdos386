# R11K known limitations

R11K is a bounded, opt-in step: a larger PC boot loader and a supported launch path for paged programs. It
is **not** a ring-3, security or isolation release, and it does not claim production readiness.

## Isolation (the important one)

1. **Kernel and applications run at CPL 0.** Page protection bits are set as a convention only; nothing is
   enforced against code that wants to write somewhere it should not. A paged program can still reload CR3,
   write kernel memory, or fault the machine on purpose.
2. **Only the 1 MiB image window is private.** The kernel heap (`qd_alloc`, libc `malloc`), every job stack
   and the kernel itself are shared and writable by all processes, at the same addresses. Blocks and stacks
   have an *owner* (reclaimed at exit), not a protection domain. Live-proven: copy A wrote a heap block, copy B
   read it (`TWICET Z`). No per-process heap was implemented.
3. A paged program's libc statics (stdout buffer, `FILE` table) are private only because they live in the
   window. Anything it allocates is shared.

## Launch path

4. **Paged programs cannot start programs.** `qd_run`, `qd_start`, `qd_pg_start`, `qd_pg_cancel`, poll/schedule
   registration and `QT_CREATE` return −62 to a paged caller. A launcher (PGRUN, TWICET, the shell) is unpaged.
5. **Fixed origin.** Only a standard slot-1 a.out (entry 0x240000, OMAGIC, no relocations, ≤ 1 MiB minus the 512-
   byte argument block) can be launched; profile 0/2/3 programs are refused with −61. No relocation, no ELF, no
   shared libraries.
6. **Capacity.** At most two simultaneous paged copies are verified on every platform (JOB_MAX 6, with SYSTEM,
   IDLE, shell and the launcher using four slots). A third is refused with a bounded code and leaves nothing
   behind. `JOB_MAX` and `PG_PROC_MAX` (3) were not raised because larger values are not independently tested.
   Frame windows: 7 (i386VM block map), 2 (PC and QEMU).
7. On the PC/QEMU map the frame pool *is* the profile-2/3 address span, so paged copies and profile-2/3
   applications (MULTIA, MULTIB, COLLIDE) exclude each other (code-reviewed; not run as a mixed test).
8. A job started **held** that is never activated and never cancelled keeps its frames and slot until the
   machine resets; there is no timeout. `qd_pg_cancel` exists for exactly that.
9. Exit status values equal to −2, −16 or −17 cannot be told from the corresponding errors (unchanged R11E rule);
   a job killed by a fault reports −17.
10. Stack size is 16 KiB per job (kernel heap), fixed; there is no guard page.

## Console

11. One console, no virtual consoles, no focus: concurrent output interleaves (whole lines normally, but
    nothing guarantees it) and **only one job may wait for input** (a second `qd_con_wait` returns −50). Input
    goes to whichever job reads next.
12. The i386VM harness has stdin at end-of-input after the script, so `qd_con_wait` returns at once there and
    the single-waiter test is reported NA on that platform (it passes on Tiny386 and QEMU).

## Faults

13. Any CPU exception (0–19) at an EIP inside a paged job's own window ends only that job; anything else (kernel
    code, unpaged jobs, NMI, double fault, machine check) halts the machine. A paged program that corrupts shared
    kernel state before faulting is not contained.
14. The i386VM CPU core ends the whole run at the first guest exception, so the fault handler is provable on
    Tiny386 and QEMU only (unchanged core limitation; the core was not patched).
15. The first line of a `PGFAULT` report prints CR2 for every vector; it is meaningful only for #PF.

## Boot loader

16. 896 sectors (448 KiB) for kernel + transfer area, hard disk (INT 13h extensions) only, A20 through port 0x92
    only. Real hardware, USB/CD/floppy boot, partitioned disks and non-512-byte sectors are **not tested**. The
    kernel's own map limits it further: text+data+BSS+heap below 0x200000.
17. The loader is hand-encoded real-mode code (the historical GAS has no `.code16`); the encoding is checked
    against a reviewed disassembly by a test that needs `objdump` (test-time dependency only).
18. Tiny386 reports a truncated image as a checksum failure (`S`), QEMU as a read error (`R`): both refuse.

## Not done (outside R11K)

Ring 3 / TSS, demand paging, swap, PAE, ELF, relocation, FAT writes / FAT32, partition-aware boot, virtual
framebuffer or graphics sessions, native GCC, SuperBASIC, a process-private heap, removal of the four linker
scripts, a removal service for registered poll/schedule tasks (the live test uses kernel-registered tasks that
are never removed). The historical `floatundidf` discrepancies in the GCC 1.40 runtime are untouched, and no
expected result was changed.
