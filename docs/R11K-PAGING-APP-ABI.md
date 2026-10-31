# R11K paged application ABI (opt-in; `QDOS_PAGING`, ABI version 2)

**Not a ring-3, security or isolation release.** Kernel and applications run at CPL 0. Paging gives each
paged process a *private image window*; it does not protect anything. The heap, job stacks, the kernel and
the devices are shared and writable by every process (section 5). Claim labels: **live-proven**,
**code-reviewed**, **not tested**; per-claim evidence is in `docs/R11K-RUNTIME-REPORT.md`.

## 1. What it is

A kernel built with `-DQDOS_PAGING` can start an ordinary GCC 1.40 a.out — linked **once**, with the unchanged
SDK default (`qdos386-gcc -slot 1`, origin 0x240000) — in a fresh address space. Every copy sees its own 1 MiB
at 0x240000–0x33FFFF; the same file started twice gives two independent images at the same virtual addresses.
Default images (no `QDOS_PAGING`) contain none of this code and are byte-identical to R11I (**live-proven** by
hash, `logs/r11k/default/identity.txt`). The old flag name `QDOS_PAGING_POC` is an `#error`.

## 2. Programs that can be launched

A paged program is an a.out that the normal loader already accepts and that is linked for slot 1:
OMAGIC, no relocation records, entry = 0x240000, text+data+BSS inside the window (the top 512 bytes are the
argument block). Nothing needs rebuilding: of the 49 programs on the R11J volume, 45 qualify as they are
(`logs/r11k/audit/aout-scan.txt`). Programs linked for another origin (slot 0 SHELL at 0x200000, profile 2/3
COLLIDE, MULTIA, MULTIB) are refused with −61 — by design they are bound to a physical profile.
The loader zeroes the whole window first, then copies text+data, so BSS is zero (crt0 does not clear it).

## 3. Starting a program

Library (`sdk/include/qdos386.h`, `sdk/lib/qdospg.c`; a separate archive member, so programs that do not use
it link byte-identically to before — **live-proven**: SAME.AOU and SAMELNCH.AOU rebuilt with the R11K library
equal their R11J bytes):

    int qd_pg_available(void);        /* 1 if this kernel can start paged programs */
    int qd_pg_caps(void);             /* (ABI version << 24) | (max spaces << 16) | QD_PG_CAP_*; or -70 */
    int qd_pg_start (name, args, flags);            /* args: one C string (one argument) or NULL */
    int qd_pg_startv(name, packed, len, flags);     /* packed: argv[1], argv[2] ... each NUL-terminated, len bytes */
    int qd_pg_cancel(int id);         /* discard a HELD job that has never run */
    int qd_pginfo(int id, struct qd_pginfo *out);   /* id 0 = caller */

`flags` is 0 or `QD_PG_HELD`. A started job is a normal job: use `qd_job_exit_status`, `qd_job_reclaimed`,
`qd_job_activate`, `qd_job_chprio`, `qd_self`. The file name is a WIN1_ root name, as for `RUN`.

**Held starts.** With `QD_PG_HELD` the job is created inactive: it owns its address space, frames and job slot
but is never scheduled until `qd_job_activate(id, 2)`. A launcher can therefore make several copies, and only
if all of them were created release them together (the PGRUN pattern) — otherwise `qd_pg_cancel` each one:
frames, record and job are released and the id goes stale. This gives all-or-nothing group starts and makes
overlap of short programs certain rather than probable. `qd_pg_cancel` refuses (−65) a job that is not paged,
is already active, or was ever dispatched, and never touches it.

**Kernel traps** (numbers are ABI; use the library): `QT_PG_START` 0x25 (`a` name, `b` packed args, `c` =
length | flags; flag `PG_START_HELD` = 0x10000; an ABI-1 kernel answers −34 to any flag), `QT_PG_INFO` 0x26,
`QT_PG_CAPS` 0x27, `QT_PG_CANCEL` 0x29. `QT_PG_TEST` 0x28 exists only in a test kernel. On a kernel without
paging every one of them answers −1, which the library maps to −70 / "not available".

**Errors** (negative): −60 no free contiguous 1 MiB frame run; −61 image not linked at 0x240000; −62 refused to
a paged caller; −63 no free address-space record; −65 not a held paged job; −34 bad arguments or unknown flag;
−20…−25 bad a.out (header, relocations, layout); −29 no free job slot; −30 no heap for the stack; −40 read
error; −44 file not found; −2 stale/unknown id. A failed start releases everything it had reserved
(address-space record, frames, job, stack) in reverse order: **live-proven** (bad header, wrong origin, missing
file, bad arguments, injected copy failure, exhaustion — and the machine state compared before/after).

## 4. Launching from the shell

`PGRUN` (`sdk/examples/r11k/pgrun.c`) is the bounded launcher:

    RUN PGRUN.AOU <copies 1..3> <program> [args...]
    RUN PGRUN.AOU 2 TWICE.AOU 30 1
    RUN PGRUN.AOU 2 CATFIL.AOU

It starts all copies held (all or nothing), activates them together, waits, and prints machine-readable lines
(`PGRUN: start`, `overlap yes|no`, `exit <i> status <hex>`, `released yes|no`, `DONE|FAILED`). Arguments reach
each copy unchanged as `argv[1…]`. A paged process cannot start programs, so there is no recursion.

## 5. What is private and what is shared

| | per paged process | shared with everything |
|---|---|---|
| image window 0x240000–0x33FFFF: text, data, BSS, libc/libqdos statics (stdio buffers, `FILE` table, `errno`), argument block | **private** (own physical frames) | |
| CPU state, x87 state, registers | per job (as always) | |
| job stack (16 KiB, kernel heap) | | **shared**: reachable by address from every process |
| `malloc`/`qd_alloc` blocks (kernel heap) | owned by the allocating job and freed when it ends | **shared**: any process can read/write any block by address |
| kernel text/data, page tables, IDT/GDT, devices | | shared, writable (CPL 0) |
| console | | shared output; one input waiter (below) |

Proof that the heap is shared and not isolated, so that nobody mistakes this for isolation: copy A writes a
kernel-heap block whose address the launcher passed, copy B reads A's value (`TWICET Z`, **live-proven**).
Everything an application must not rely on being private (heap, stack) is therefore stated as shared here and
in `docs/R11K-KNOWN-LIMITATIONS.md`. No per-process heap was implemented.
The kernel refuses to use a paged caller's pointer outside its own window or the shared heap (−14), so a
paged program cannot pass a kernel a pointer into the unmapped pool: **live-proven** (R11J ranges test).

## 6. Services from a paged process

Working (**live-proven** in TWICE/CATFIL under two concurrent copies): console output (`printf`, `putchar`),
FAT16 read-only channels (`fopen`/`fread`/`fseek`/`fclose`, 192 KiB file read by two copies at once),
`qd_alloc`/`malloc`/`free`, `qd_ticks`, `qd_yield`, `qd_self`, `exit` or return from `main`, x87 (an
accumulator survives preemption and CR3 switches bit-exactly), `qsort`, `sprintf`.
Refused with −62 to a paged caller: `QT_CREATE`, `QT_POLL_ADD`, `QT_SCHED_ADD`, `QT_IMG_LOAD` (`qd_run`),
`QT_IMG_START` (`qd_start`), `QT_PG_START`, `QT_PG_CANCEL` — each would make the kernel use the caller's private
memory later from another address space. **Live-proven** (`TWICET C R`).

**Console input.** There are no virtual consoles. Output of concurrent copies interleaves (line-buffered
stdout keeps whole lines together; nothing guarantees it). Only one job may wait for console input: a second
`qd_con_wait` returns −50, a first waiter is released by input or by `qd_job_reactivate`. **Live-proven** on
Tiny386 (`TWICET W`); not runnable on the i386VM harness, where end-of-input is already pending (reported NA).

## 7. Faults

Any CPU exception (vectors 0–19) raised at an EIP inside a paged job's own window ends **that job only**: a
report starting `PGFAULT: vec …` is printed, the job's exit status is `QD_ERR_NOSTATUS` (−17), its resources
are reclaimed like any exit, and every other job continues. Page fault, divide error and invalid opcode are
**live-proven** on Tiny386 beside a healthy copy; QEMU: page fault in the R11J-style scripts and TWICET F.
The job is blamed only if its **own directory was the active one** when the exception was taken: addresses
0x240000.. also hold *unpaged* slot-1 code, and poll/schedule tasks run under the template directory, so a fault
in such code while a paged job happens to be current halts the machine instead (found in the independent review,
reproduced on the pre-fix kernel and fixed; `TWICET Y`, `logs/r11k/review/`). An exception anywhere else (kernel
code, unpaged jobs), NMI, double fault and machine check halt the machine as before. `cr2` in the report is meaningful only for vector 14. The i386VM CPU core aborts the whole run at any
guest exception before delivery, so the handler cannot be exercised there (a known core limitation, unchanged).

## 8. Capacity

`JOB_MAX` 6 (SYSTEM, IDLE, shell, launcher, plus two applications), `PG_PROC_MAX` 3 address-space records,
frame windows: 7 on the i386VM block map, 2 on the PC/QEMU map. Therefore two simultaneous paged copies are the
tested maximum on every platform; a third is refused with a bounded code (−29 job slots on i386VM, −60 frames on
PC/QEMU) and the refusal leaves nothing behind. On the PC map the frame pool is the profile-2/3 span, so paged
copies and profile-2/3 applications exclude each other (code-reviewed; the region allocator refuses the second
user). `JOB_MAX` was not raised: a larger value would have been untested.

## 9. Unchanged

R11J's `QT_PG_START`/`QT_PG_INFO` numbers and results, SAME/SAMELNCH, the deferred hold-flag write, template
execution of poll/schedule tasks, the shared-kernel mapping and scheduler switch, and every non-paging path.
