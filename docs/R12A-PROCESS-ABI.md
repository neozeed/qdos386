# R12A process ABI

Kernels built with `QDOS_DYNPROC` (`make r12a-pc`). Library: `sdk/lib/qdosproc.c` (libqdos member `qdosproc.o`),
declarations in `sdk/include/qdos386.h`. Trap numbers and kernel constants: `kernel/jobs.h`, `kernel/proc.h`.

**Private memory is not security isolation.** Every process runs at CPL 0 and can execute privileged instructions or write
any mapped address. What R12A guarantees is memory *correctness and ownership*: each process has its own frames for its
image, heap and stack; the kernel only reads and writes a process's memory on its behalf through validated ranges and in
its own directory; everything a process owns is returned when it ends.

## 1. What a program sees

* **Same binaries.** An unchanged a.out (OMAGIC, no relocations) runs at its link origin. 0x240000 is the canonical origin
  (`qdos386-gcc` default); 0x200000, 0x380000 and 0x500000 (historical profiles) also run, each copy in its own frames.
  Any number of copies of one program may run at once (bounded by the job table and frames).
* **Startup is unchanged** (R8): `_start` gets argc/argv from QT_APP_ARGS (block at the top of the historical window),
  calls `main`, passes its value to QT_EXIT and returns. It now runs on a **private 64 KiB user stack** at
  0x1FFF0000..0x20000000 instead of a kernel-heap block.
* **Heap.** QT_MEM_ALLOC / QT_MEM_FREE (libc `malloc/calloc/realloc/free`, `qd_alloc/qd_free`) allocate in the calling
  process's private heap (0x10000000 up, 8 MiB virtual, grown on demand by explicit mapping). Same error codes as before
  (-3 out of memory, -6 bad pointer, -7 double free, -8 other owner). QT_MEM_VALIDATE checks the kernel heap and the
  caller's heap. QT_MEM_FREE_BYTES still reports the **kernel** heap (the system figure LEAK.AOU tracks).
* **Pointers given to the kernel** must lie entirely inside one of the caller's own areas: image pages, argument page,
  heap up to its current end, user stack. NULL, kernel memory, the shared kernel heap, unmapped window pages, ranges that
  cross an area boundary or wrap are refused with -14 (CHAN_ERR_RANGE) or the call's own error.
* **Faults** raised by a process's own code (EIP outside the kernel image while its directory is active) end that process
  only; its exit status is "ended without a status" (-17). A user-stack overflow is caught through the #DF task gate and
  ends only that process. The report on console 0 / the diagnostic stream: `PGFAULT: vec err cr2 eip job cr3` + registers.
* Still refused to a process (-62): QT_CREATE (a job at an address in the creator's space), QT_POLL_ADD, QT_SCHED_ADD.

## 2. Process services (new traps; unknown traps answer -1 on older kernels → `QD_PROC_ERR_NONE` -71)

| Trap | libqdos | Meaning |
|---|---|---|
| 0x2F QT_PROC_CAPS | `qd_proc_caps()` | `(1 << 24) \| (16 << 16) \| caps`: DYNAMIC 1, PHEAP 2, PSTACK 4, SPAWN 8, GUARD 0x10 |
| 0x30 QT_PROC_SPAWN | `qd_spawn(name, packed, len, flags, console)` | start a child: request block {version 1, packed args, length, flags, console}; flags `QD_SPAWN_HELD` (created inactive); console −1 = the caller's, 0..3 = that console. Returns the tagged child id or a negative code |
| 0x31 QT_PROC_WAIT | `qd_wait(id, ticks)`, `qd_poll(id)` | exit status of a child (≥ 0, or -17 no status, -66 killed); `ticks` −1 = forever, 0 = poll; -70 timed out / still running; -68 not a child of the caller |
| 0x32 QT_PROC_INFO | `qd_proc_info(id, &info)` | 24 words: version, id, parent, paged, space (directory), origin, image end, args, heap base/brk/cap/used, stack lo/hi, frames held, pool frames/free, kernel stack bytes/high-water, dispatches, preempts, faults, children, live processes |
| 0x33 QT_PROC_KILL | `qd_kill(id)` | end a child (a held child that never ran is released at once; its status becomes -66) |
| 0x34 QT_PROC_VA | `qd_proc_va(p)` | diagnostic: the PTE (frame \| flags) that maps p in the caller's space |

Blocking: a QT_PROC_WAIT that must wait puts the caller to sleep in the scheduler (other jobs and consoles keep running)
and, when the child ends or the timeout expires, returns -73 (`PROC_ERR_AGAIN`); `qd_wait` repeats the call with the
remaining time. The kernel never writes into a sleeping parent's memory.

## 3. Parent, child, status retention

* A **record** {parent, child, status} is created by QT_PROC_SPAWN only. When the child ends, the reclaim of its job
  stores the exit status in the record and wakes a waiting parent. The record stays until the parent waits/polls it
  (**zombie retention**; the child's memory, job slot and frames are already free) or the parent itself ends.
* **Orphans.** If the parent ends first its records are dropped; running children go on and nobody collects their status.
* Limits: 8 unreaped children per parent (-69 `PROC_ERR_LIMIT`), 32 records in the system, 16 jobs.
* Ids are generation-tagged `(tag << 16) | slot`; a stale id is never a child (-68).
* **Inheritance.** A child gets a copy of the parent's current directory (R11M), the parent's console or the console
  named in the request, no open channels (every program opens its own CON/files; there is no handle inheritance), no
  environment (none exists yet). Raw parent pointers are never passed: the argument strings are copied by the kernel into
  the child's argument page.
* **Console focus.** One reader per console (R11M). A child that reads input should be given its own console; on the
  parent's console it would compete with the parent (the second reader is refused, -50). A parent waiting in
  QT_PROC_WAIT does not read, so an execute-and-wait child may use the parent's console.

## 4. Compatibility veneers (unchanged binaries)

| Old service | R12A behaviour |
|---|---|
| QT_IMG_LOAD (shell RUN) | a new process (image linked for 0x240000; -32 otherwise, as before) |
| QT_IMG_START (profile 0..3 / −1) | a new process at that image's origin; a profile mismatch gives the old codes (-32/-33/-35); **no -28 "slot busy"**: two programs at one origin no longer collide |
| QT_PG_START / QT_PG_CANCEL / QT_PG_INFO / QT_PG_CAPS | same layout; PG_CAPS reports ABI version 3 and 14 spaces; PG_INFO's frame base is the frame of the first image page |
| QT_JOB_EXIT / QT_JOB_RECLAIMED / QT_JOB_QUERY / QT_JOB_KILL | unchanged; now usable from any process |
| shell RUN/START/PS/WAIT/KILL | unchanged commands; the shell itself is a process (origin 0x200000) |

## 5. SDK

`qdos386-gcc` links for 0x240000 by default; `-slot N` remains only for the historical test images and is not needed
for new programs. New programs use `qd_spawn/qd_wait/qd_poll/qd_kill/qd_proc_info` from libqdos.
