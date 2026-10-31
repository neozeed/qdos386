# R12A known limitations, residual risks and NOT TESTED list

Evidence classes as everywhere: **host test** (strict C89 on the build host), **guest executed** (QEMU full machine unless
stated), **independent comparison** (mtools, `fsck.fat`, `tools/r11m-fatwalk.py --vfat`), **not tested**, **blocked**.

## 1. Privilege — the residual exposure (by design of this milestone)

* **No security isolation.** All code runs at CPL 0 with flat 4 GiB segments. A process can execute any privileged
  instruction (`cli`, `lgdt`, `mov %cr3`, port I/O), write any address that is mapped in its directory — which includes
  the whole kernel, the kernel heap, the IDT/GDT/TSS and every identity-mapped device — and so corrupt the kernel or, by
  reloading CR3, reach other processes' frames. R12A provides private memory for **correct programs** (no accidental
  sharing, ownership and reclaim, faults confined to their process), not protection from hostile ones.
* No W^X: text pages are writable (the 386 ignores R/W at CPL 0; CR0.WP is not set).
* User pointers are range-checked against the caller's own areas; at CPL 0 a range check cannot make a pointer safe
  against a program that bypasses the kernel.
* The next step to real isolation is a ring-3 conversion (user code and data segments, a TSS ESP0 per job, U/S bits that
  mean something, `iret` to CPL 3); out of scope here.

## 2. Memory and process limits

* At most 16 jobs (SYSTEM, IDLE, the shell + 13); 8 unreaped children per parent; 32 child records in the system.
* Physical memory used: at most 16 MiB (identity map); pool 3,264 frames on the 16 MiB QEMU harness, 1,216 on 8 MiB
  Tiny386. Exhaustion is a stable error (-60), never a hang (guest executed: BIGBSS).
* Each process commits its whole image and a 64 KiB user stack at creation (no demand paging, no swap: excluded). The heap
  grows explicitly and never shrinks; frames return only when the process ends.
* Kernel stacks are 16 KiB heap blocks without a guard page; overflow is detected only by the canary check on switch-out
  (`stack_faults`). Measured high-water mark of busy processes: see the runtime report.
* Only the four historical link origins are accepted (0x240000 canonical; 0x200000, 0x380000, 0x500000). No relocation:
  a.out files with relocation records are refused as before.
* The argument block stays 512 bytes at the top of the historical window (R8 ABI).
* Physical frames are not cleared when freed; they are zeroed when they are given to a process (so no data leaks between
  processes through frames).

## 3. Process model

* No handle inheritance: a child starts with no open channels (every program opens CON and its files itself). Close-on-
  exec is therefore moot.
* No environment variables, PATH or startup scripts (the next usability milestone).
* Orphans keep running; their exit status is not collected by anyone.
* A process cannot create a job at an address in its own space (QT_CREATE), nor register poll/schedule tasks.
* SuperBASIC has no `EXEC`/`EXEC_W` syntax yet; the service exists (QT_PROC_SPAWN/WAIT) and is tested from C
  (SPAWNT.AOU).
* The historical NEG test expects `-28 slot busy` for a second image at an occupied profile. Under R12A two images at one
  origin no longer collide — that is the point of the milestone — so those checks report the new behaviour (runtime report
  §NEG); the invariant they protected (no two programs share memory) is proven by MAPTEST instead.
* The shell is the R11M/R12 shell, unchanged in behaviour; on R12A volumes it is built with a 16-entry PS buffer
  (`shell-a.aout`). The R12 shell binary (8 entries) also runs on R12A but its PS/KILL /CON see at most 8 jobs.

## 4. Faults

* Recoverable: any exception raised by a process's own code (EIP outside the kernel image, its directory active), and a
  user-stack overflow (#DF task gate). Only that process ends (status -17).
* Not recoverable (the machine halts with a report): faults in kernel code, a fault inside the #DF handler, NMI, machine
  check.
* A process that corrupts kernel memory (possible at CPL 0) is not detected.

## 5. NOT TESTED

* A #DF that is not a user-stack overflow (e.g. a kernel-stack overflow into unmapped memory) — would halt by design.
* Frame exhaustion during heap growth while other processes are being created at the same moment (exhaustion in a single
  process is tested on the host and in the guest).
* Process creation with exactly the last free frames at every possible failure point **in the guest** (covered by the host
  rollback test at every point; the guest exercises the -60 path).
* Tiny386 beyond the single bounded smoke (no #DF, no fault tests there).
* Power-cut behaviour, FAT16 crash atomicity (no claim).
* Zork (not supplied for this round).
* CPU-only i386VM (retired), guest-native GCC (out of scope).
