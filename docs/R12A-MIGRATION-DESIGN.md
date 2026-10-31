# R12A migration design — dynamic process address spaces

Input: `R12A-MEMORY-AUDIT.md`. This is the plan the stages implement; deviations are recorded in the stage documents and
`R12A-RUNTIME-REPORT.md`.

## 1. Build identity and rollback

* New opt-in kernel option **`QDOS_DYNPROC`** (implies `QDOS_PAGING`), PC/QEMU only: `make r12a-pc` builds
  `build/r12a/pc-vga/r12a-vga.img` (QEMU) and `build/r12a/pc/r12a.img` (headless, Tiny386).
* Everything new lives in new files (`kernel/frames.c`, `vas.c`, `pheap.c`, `proc.c`, `kctx.s`) plus `#ifdef QDOS_DYNPROC`
  hooks in `jobs.c`, `aout.c`, `imgload.c`, `channel.c`, `descriptors.c`, `shellboot.c`. `paging.c`/`paging.s` and
  `context.s` are not linked into R12A kernels and are unchanged.
* **Rollback = the R12 kernel**, still built by `make r12-pc-pg` (the "legacy fixed-slot mode" used for comparisons). Default
  images stay byte-identical to R11M-FIX1 (`scripts/r11l-default-check.sh`, six hashes) — checked at every stage.
* A runnable checkpoint is committed after each stage; a later stage never rewrites an earlier commit.

## 2. Physical memory

```
 0x000000 .. 0x1FFFFF  kernel, BIOS, VGA, kernel heap                  identity, supervisor, every directory
 0x200000 .. 0x23FFFF  shell slot (Stages 1-2); frame pool once the shell is a process (Stage 3)
 0x240000 .. 0x33FFFF  frame pool (former slot 1)
 0x340000 .. 0x37FFFF  transfer area                                    identity, every directory
 0x380000 .. 0x6FFFFF  frame pool (former profiles 2/3 / R12 paged pool)
 0x700000 .. 0x7FFFFF  boot stack (SYSTEM) and BIOS data                identity, every directory
 0x800000 .. RAMTOP    frame pool (RAMTOP = 1 MiB + CMOS 0x30/0x31 KiB, capped at 16 MiB = the identity map)
```

* **Frame allocator (`frames.c`)**: one bit per 4 KiB frame below 16 MiB (512-byte bitmap) plus a pool-membership table of
  ranges; per-owner counts and a global used/free count that must always add up; every frame is pattern-tested once at
  boot before it joins the pool (measured, not assumed). Frames are handed out singly: **no contiguity**.
* Page directories and page tables of processes are frames from the same pool.
* The template directory (identity 0..16 MiB, supervisor) stays in kernel BSS.

## 3. Virtual layout of a process (one directory per process)

```
 0x00000000 ┌──────────────────────────────┐
            │ kernel, heap, VGA (identity) │ shared tables or private copies of the template table
 0x00200000 ├──────────────────────────────┤ ─┐ pool VA: in a process directory every pool page is either
            │ image window (origin..)      │  │ the process's own frame or NOT PRESENT
            │   text | data | bss  (pages) │  │   origin 0x240000 (canonical), or 0x200000/0x380000/0x500000
            │   ...unmapped...             │  │   (historical origins, run unchanged at their link address)
            │   argument page (top - 4 KiB)│  │
 0x00340000 ├──────────────────────────────┤ ─┘
            │ transfer area (identity)     │
 0x00380000 ├──────────────────────────────┤   pool VA (private or not present)
 0x00700000 ├──────────────────────────────┤
            │ boot stack (identity)        │
 0x00800000 ├──────────────────────────────┤   pool VA: not present
 0x01000000 ├──────────────────────────────┤
            │ (unmapped)                   │
 0x10000000 ├──────────────────────────────┤   private heap: grows upward page by page (explicit growth on
            │ heap                         │   QT_MEM_ALLOC, not demand paging), capped per process
            │ ...unmapped...               │
 0x1FFF0000 ├──────────────────────────────┤   user stack, 64 KiB, fully committed at creation
 0x20000000 └──────────────────────────────┘   everything below the stack down to the heap cap is unmapped = guard
```

* The image is mapped page by page for exactly text+data+BSS (rounded up); pages of the window beyond it are not present.
* The argument block keeps its R8 position (top 512 bytes of the historical window: 0x33FE00 for 0x240000 images).
* Image limits as before (a 0x240000 image ≤ 0xFFE00 bytes) and every a.out check of R8 (magic, no relocations, alignment,
  `_start` == entry, exact file size) is kept; new checks: origin is one of the four, image end inside the window.
* No W^X: at CPL 0 the 386 ignores R/W, and CR0.WP (486+) is not set because a kernel copy-out to a read-only page would then
  fault in the kernel. Text pages are mapped writable; this is recorded as a limitation.

## 4. Kernel entry stacks (the dangerous part)

Rule from the audit: the scheduler loads CR3 while executing on the current stack, and nothing in a ring-0 interrupt gate
switches stacks. R12A therefore makes **every kernel entry run on a per-job kernel stack in shared (template-mapped) memory**:

```
  application code on the PRIVATE user stack (CR3 = process)          kernel stack: kernel-heap block, mapped everywhere
  ─────────────────────────────────────────────                       ──────────────────────────────────────────────
  int/IRQ/exception: CPU pushes eflags,cs,eip on the user stack       ┌ job_trampoline/job_run/proc_entry frames ┐
  stub: pushal (user stack), eax = esp                                │                                           │
        if esp not inside [kx_lo, kx_hi): esp = kx_entry  ───────────▶│ kx_entry: pushl eax (pointer to frame)    │
        call C(frame)  -- C reads/writes the frame in the CALLER's    │ C handler / scheduler frames              │
                          space, before any CR3 change                │   (CR3 may change here: we are on shared  │
        C returns next job's saved frame | fmt                        │    memory)                                │
  resume: esp = that frame (now mapped: CR3 already switched); popal; iret
```

* `kx_lo/kx_hi/kx_entry` are globals for the current job, reloaded by `schedule()` on every switch. For jobs without a
  private stack (SYSTEM, IDLE, any template job) the range is the whole address space, so no switch happens and behaviour is
  exactly R12's.
* A process starts in kernel code on its kernel stack (`job_trampoline → job_run → proc_entry`); `proc_entry` records
  `kx_entry` (just below its own frame) and calls `_start` on the user stack; when `_start` returns, ESP goes back to the
  kernel stack and `job_exit` runs there. `yield_asm` is only ever reached on the kernel stack.
* Nesting: no trap re-enables interrupts (audit section 5), so a kernel stack holds at most one entry frame plus kernel
  frames; an interrupt arriving while the job runs kernel code on its kernel stack does not switch (it is already inside
  the range).
* Kernel stack size 16 KiB for processes (was 64 KiB of heap per program); high-water mark measured.
* Exceptions: a fault whose saved CR3 is the faulting job's directory and whose EIP is in the job's image, or whose CR2 is
  in its private ranges, ends only that job (report: vector, error, CR2, EIP, CR3, job id). Anything else halts.
* **Stack overflow into the guard**: the CPU cannot push the #PF frame onto the guard page, so it raises #DF. #DF is routed
  through a **task gate** to a separate TSS with its own stack and the template CR3. The handler checks that the faulting
  task's CR3 is the current process's and its ESP is in that process's guard/stack range; it then rewrites the saved state of
  the interrupted task so that `iret` resumes in a small stub on the job's kernel stack that ends the job and schedules.
  Any other #DF halts with a report. (Task switching exists in both emulators: QEMU, and Tiny386 `i386.c task_switch`.)

## 5. Cross-directory rule (fix of the R11J class at the boundary)

* **Pool memory is touched by the kernel only under the template directory**, entered and left explicitly
  (`vas_enter_template()` / `vas_leave_template()`), and only while running on a kernel stack. Building, growing and
  tearing down address spaces, and reading page tables for reports, all happen inside such a section. While inside, the
  kernel never touches the caller's private memory (the trap frame lives on the user stack): code reads arguments before and
  writes results after.
* Writes into a process's private memory made on behalf of that process (trap results, channel reads, QT_APP_ARGS) happen in
  its own trap, under its own CR3.
* Asynchronous writes (hold flags from the scheduler, IRQ1 wake-ups, another job's REACTIVATE) go through the existing
  deferral: a flag at a private address is written only when the owner's directory is active. "Private address" is now
  decided by the process's own range table, not by a fixed window.
* Poll/schedule tasks and QT_CREATE remain refused for processes (they would make the kernel call code in a private space).

## 6. User pointers (copy-in / copy-out policy)

A process may pass a pointer only into its own committed ranges: image pages, argument page, heap [base, brk), user stack.
`chan_user_range_ok` checks `[addr, addr+n)` against that range table (overflow, NULL and zero-length handled; a range may
not straddle two areas). The shared kernel heap is no longer acceptable from a process. This is a correctness check, not a
security boundary: at CPL 0 a program can bypass the kernel entirely.

## 7. Private heap

`QT_MEM_ALLOC/QT_MEM_FREE` from a process allocate in its private heap (`pheap.c`: first-fit, address-ordered, coalescing,
16-byte headers with magic and size, same error codes as the kernel heap). The allocator's metadata lives in the process's
pages and is only touched in that process's own trap. Growth maps new zeroed frames at the heap top (template section),
up to the cap. So **unchanged binaries** (libc `malloc` = QT_MEM_ALLOC, BASIC's 256 KiB interpreter stack) get private
heaps without relinking. QT_MEM_FREE_BYTES keeps reporting the kernel heap (the system-wide figure LEAK.AOU measures).
Exit frees the whole heap with the address space; nothing is walked.

## 8. Processes, creation, wait (Stage 3)

* `proc_spawn(name, packed args, flags, console)`: validate (reads only) → job slot + kernel stack → directory → image
  frames + copy → argument page → heap/stack frames → bind. Every step undoes all earlier steps on failure (frames, tables,
  directory, job record, channel references); exhaustion returns a stable error (`PROC_ERR_NOFRAMES` -60, …).
* Shell traps QT_IMG_LOAD/QT_IMG_START/QT_PG_START become veneers over `proc_spawn`, so the existing shell binary uses the
  common path. QT_PG_INFO/CAPS/CANCEL keep their layouts with dynamic contents (ABI version 3).
* New versioned traps (QT_PROC_CAPS/SPAWN/WAIT/INFO/KILL) usable from any process: parent/child records with exit-status
  retention in the parent, blocking wait that sleeps in the scheduler, poll (wait with timeout 0), orphan auto-reaping,
  generation-safe ids, per-parent child limit.
* `JOB_MAX` 16 in R12A kernels (bounded table; job memory dynamic).
* The shell itself becomes a process (origin 0x200000) once creation from processes is proven; physical slot 0 then joins
  the pool and no fixed application slot remains.

## 9. Risk register

| Risk | Impact | Mitigation |
|---|---|---|
| Entry stub switches to the wrong stack, or a frame is touched after a CR3 change | triple fault | single stub macro reviewed line by line; QEMU `-d int,cpu_reset` on first runs; tiny guest tests before real apps |
| #DF task gate mis-set (TSS fields, busy bit) | triple fault on overflow only | test only with the guard test program; normal operation never takes the gate |
| Tiny386 differs (task switch, CMOS) | smoke fails | the smoke does not provoke #DF; CMOS fallback to the 8 MiB map |
| Kernel heap exhaustion (16 KiB per kernel stack, 16 jobs) | spawn refused | stable error; measured free heap reported |
| Frame pool exhaustion mid-spawn | partial process | transactional build, host tests with failure injection at every allocation |
| Kernel writes to a private page while another CR3 is active | silent corruption | template sections only on kernel stack; range-table based deferral; MAPTEST/HEAPT cross-checks |
| Old tests that assume fixed slots (NEG contention, R11K refusal checks) | false FAILs | label the assumption; dynamic equivalents (`docs/R12A-RUNTIME-REPORT.md`) |

## 10. Test plan (bounded)

Host (strict C89, `tests/host/r12a/`): frame accounting, mapping construction/teardown with simulated physical memory,
rollback at every failure point, exhaustion, reuse, private heap allocator, user-range checks.
Guest (QEMU primary): MAPTEST (same VA, distinct frames/CR3), HEAPT (private heap churn), STKT (deep recursion + guard),
FAULTT (invalid access/opcode/divide with a sibling running), SPAWNTEST (spawn/wait/poll/kill matrix), then the R12 suites
affected (Stages 1-4, demo) on the R12A kernel, NEG/MULTI with labelled dynamic expectations, one Tiny386 smoke.
