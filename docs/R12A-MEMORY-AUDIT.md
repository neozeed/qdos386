# R12A Phase 0 — memory and process audit of frozen R12

Base: R12 `0e92a742291a47067823d42783ee3eb366d02b83` (archive `qdos386-R12-INTERACTIVE-OS-STRETCH.tar.gz`,
SHA-256 `add9df46a7fba37ea6bbcf881b92744cf0a31719e3635967bf109382d1881e85`, both re-verified). Branch `r12a-dynamic-processes`.
Audited from the source and from the shipped binaries, not from earlier documents.

## 1. Baseline verification

| Item | Result |
|---|---|
| HEAD of the branch point | `0e92a742…` (`git rev-parse`) |
| Archive hash | matches the delivery hash |
| QEMU kernel `build/r12/pc-pg-vga/r12-pg-vga.img` | SHA-256 `dd11fe87f398d6b30faaf6df16ff23296430edc2a0d4e3a69172076fd1b9317a`, identical to the archived copy |
| Short QEMU smoke on R12 (shell, editor, BASIC, Phoon, NEG, MULTI) | `logs/r12a/phase0/` (results in section 9) |

## 2. Kernel layout (R12 QEMU kernel, measured from `kernel.aout`)

| Item | Value |
|---|---|
| text / data / BSS | 0x15610 / 0x264 / 0x26968 bytes; kernel linked at 0x100000 |
| BSS end (`_bss_end`) | 0x13C1E0 |
| Kernel heap arena | 0x13C1E0 .. 0x200000 = **802,336 bytes** (one arena for job stacks, channels, kernel objects and every application `malloc`) |
| Paging table pool in BSS (`pg_mem`) | 86,016 bytes at 0x121AFC (template + 3 fixed records) |
| Free heap at the idle shell prompt | 667,024 bytes (R12 `LEAK.AOU`) |

## 3. Physical memory map in use (R9 map + R12 `QDOS_PG_WIN3`)

```
 0x000000 .. 0x0FFFFF  real-mode area, BIOS, VGA 0xB8000          (identity, all directories)
 0x100000 .. 0x13C1DF  kernel text/data/BSS                         (identity, all directories)
 0x13C1E0 .. 0x1FFFFF  kernel heap: job stacks (64 KiB each), channels, malloc blocks
 0x200000 .. 0x23FFFF  slot 0 / profile 0: the shell, unpaged       FIXED
 0x240000 .. 0x33FFFF  slot 1 / profile 1: RUN images, unpaged      FIXED (also the paged window VA)
 0x340000 .. 0x37FFFF  transfer area (boot loader puts the xfer directory here)
 0x380000 .. 0x6FFFFF  paged frame pool: three contiguous 1 MiB runs (profiles 2/3 when unpaged)   FIXED
 0x700000 .. 0x7FFFFF  boot stack (SYSTEM, top 0x7FFFF0) and BIOS data
 0x800000 .. RAM top   unused (QEMU harness: -m 16 → 8 MiB idle; Tiny386 smoke: mem_size 8M → none)
```

RAM size: neither kernel reads it. Both emulators fill CMOS 0x30/0x31 with the extended memory size in KiB (QEMU/SeaBIOS
standard; Tiny386 `misc.c cmos_init`), so it can be measured at boot. R12A measures it (section 8 of the design).

## 4. Fixed addresses, profile checks and shared resources (every one found)

| # | Where | What is fixed or shared | Consequence |
|---|---|---|---|
| 1 | `kernel/appmap.h` | slot/profile bases 0x200000, 0x240000, 0x380000, 0x500000; sizes; `HEAP_END` = slot 0 base; `USER_RANGE_END` | all loaders and pointer checks derive from these |
| 2 | `aout.c aout_check_src` | entry must equal a profile base (`profile_of`); `APP_IMAGE_LIMIT_OF(slot)` | the profile is chosen by the link origin |
| 3 | `aout.c app_load_src` | `slot_owner[]`, `region_reserve_at(base)`: one image per profile, physical = virtual | a second copy of a program is refused (-28) |
| 4 | `aout.c app_load_src` / `app_load_paged` | argument block at the top of the profile (0x33FE00 for slot 1) | ABI detail kept by R12A for 0x240000 images |
| 5 | `aout.c app_load_paged` | entry must be 0x240000; frames are one **contiguous** 1 MiB run (`app_frames_alloc` steps through the pool) | at most 3 paged programs; fragmentation refuses a 4th |
| 6 | `paging.c` | window `PG_VBASE/PG_VSIZE` fixed; `PG_PROC_MAX` 3; static `pg_mem` for 3 records; pool `PG_POOL_LO..HI` fixed | |
| 7 | `jobs.c job_alloc_sized` | every job stack is a kernel-heap block (`APP_STACK_BYTES` = 64 KiB in R11L+ kernels) | 64 KiB of the 0.8 MB heap per program |
| 8 | `jobs.c schedule` | the CR3 load happens while still running on the **outgoing** job's stack; then ESP is set to the incoming job's saved frame | every stack must be mapped in every directory → stacks must be shared memory |
| 9 | `context.s` (irq0, trap, irq1, irq7, irq15, yield) and `paging.s` (exceptions 0..19) | no stack switch on entry: CPU frame + `pushal` go onto whatever stack is current | a private application stack would be under the scheduler when CR3 changes (the R12 BASIC triple fault) |
| 10 | `descriptors.c` | GDT null/code/data only, no TSS; exceptions are interrupt gates | a stack overflow into an unmapped page faults again while pushing the #PF frame → #DF on the same stack → triple fault |
| 11 | `jobs.c trap_dispatch` | paged callers refused: QT_CREATE, POLL_ADD, SCHED_ADD, JOB_KILL, IMG_LOAD, IMG_START, PG_START, PG_CANCEL | only unpaged jobs (the shell) can launch or kill |
| 12 | `channel.c chan_user_range_ok` | paged caller: window or shared heap; unpaged caller: `arena_base .. USER_RANGE_END` (flat) | a range check, not protection (ring 0) |
| 13 | `sdk/libc/malloc.c` | every `malloc` is a kernel-heap block (QT_MEM_ALLOC) owned by the job | applications share one 0.8 MB heap with the kernel |
| 14 | `apps/superbasic/qdos/qb_stack.c` | 256 KiB interpreter stack from `qd_alloc` (shared heap) by necessity (rule 8) | |
| 15 | `shellboot.c` | the shell is loaded unpaged into slot 0 and runs in the template directory | a fixed slot |
| 16 | `jobs.h` `JOB_MAX` 6; `vcon.c VCON_JOBS` 8; shell `JC_MAXJOBS` 8 (PS buffer) | bounded tables | 3 application jobs besides SYSTEM, IDLE, shell |
| 17 | `jobs.c exit_rec[slot]` | exit status kept per slot, replaced when the slot is reused | no parent/child retention |
| 18 | `imgload.c img_packed` | static name+argument buffer (traps do not nest) | fine under IF=0 traps |

## 5. Dereferences made while another directory may be active (the R11J class)

| Site | Context | R12 handling |
|---|---|---|
| hold-flag write (`HOLD_RELEASE`) from `pick_next` (timer), `vcon_wake_job` (IRQ1), QT_REACTIVATE (another job's trap) | any CR3 | `pg_hold_release`: written at once if the flag is outside window/pool, else deferred to `pg_after_switch` of the owner |
| poll and schedule-list tasks | timer ISR / scheduler | run under the template (`run_tasks_template`); paged jobs may not register |
| trap frame `f[]` | written by `trap_dispatch` before `schedule` | on the caller's stack, caller's CR3 — correct |
| exception report | `pg_exception` | uses only `con_putc` (VGA identity-mapped) |
| reclaim of a dead paged job | another job's trap | frees frames by region handle, no dereference of the dead space |
| QT_APP_ARGS | caller's trap | reads `jobs[].args` in the caller's own space |
| channel I/O, names, `jobctl_note_launch`, `img_load_request` | caller's trap | synchronous copy in the caller's space |

No kernel code re-enables interrupts inside a trap (`grep sti`: only the resume paths), and `yield_asm` is called only from
`job_exit`. Kernel work is therefore never nested: an interrupt arrives only in application code or in job-level kernel
code (`job_run`, idle loops). This makes an explicit kernel-entry stack switch tractable (design section 4).

## 6. Binaries and link origins (inventory of the shipped a.out files)

All 70 inspected images are OMAGIC 0407 with **no relocation records**. Origins found:

| Origin | Images |
|---|---|
| **0x240000 (canonical)** | every SDK program, R12 EDIT/BASIC/LEAK/CONAPI…, Phoon, NEG, MULTI, PGRUN, TWICE… (66 files) |
| 0x200000 | the shells (`SHELL.AOU` R11K 3,600 B text; `shell-m.aout` 19,696 B) |
| 0x380000 | `COLLIDE.AOU`, `MULTIA.AOU` (R11H profile-2 tests) |
| 0x500000 | `MULTIB.AOU` (R11H profile-3 test) |

Largest image: `EDIT.AOU` text 33,200 + data 1,280 + BSS 271,456. The SDK linker scripts are `sdk/ld/slot0..3.ld`;
`qdos386-gcc` defaults to `-slot 1` (0x240000). R12A therefore keeps 0x240000 as the canonical origin and must run the other
three origins unchanged at their link address.

## 7. Process entry/exit ABI as implemented

* The loader creates a job whose `entry` is the image's `_start`; `job_trampoline → job_run` calls it as a C function on the
  job's heap stack. `_start` (crt0) does QT_APP_ARGS, calls `main`, QT_EXIT with its value, and **returns**; `job_run` then
  calls `job_exit`. `qd_exit`/`qd_end_job` unwind to `_start`'s frame (`_qd_entry_frame`) and return the same way.
* No environment. Arguments: a 512-byte block (argc, argv array, strings) at the top of the profile.
* No parent/child relation in the kernel: the shell keeps its own table and polls QT_JOB_EXIT / QT_JOB_RECLAIMED.
* Job ids are `(tag << 16) | slot`, tag incremented on every reuse: stale ids are rejected (`job_lookup`).
* The shell, editor and BASIC pass only pointers into their own image/BSS or their kernel-heap blocks to traps.

## 8. Privilege

Everything runs at CPL 0 with one flat code and one flat data segment. U/S bits protect nothing; the 386 ignores R/W at CPL
0 (CR0.WP does not exist on the 386 and is not set). Any private-memory design in this milestone is **memory correctness and
resource ownership, not security isolation**: an application can still execute privileged instructions, reload CR3, or write
any mapped address.

## 9. Phase 0 smoke on R12 (QEMU, guest executed)

Logs in `logs/r12a/phase0/`.

| Run | Result |
|---|---|
| R12 integrated demo re-run (shell, `DIR /W /P`, history, EDIT save, BASIC on another console, ticker, Phoon, PATHT 68, STRTST 37, reboot persistence, fsck, VFAT walker) | 27/27 |
| `NEG.AOU` (malformed images, contention, churn, invalid ids) | first attempt: 848 checks, 10 failures — the scratch volume lacked the ten malformed files that `scripts/r11h-negative-volume.sh` adds (test setup, same cause as the R11H QEMU note). With them (`tests/r12a/add-neg-files.sh`): **848 checks, 0 failures, PASS** |
| `MULTI.AOU` (profile 2/3 children) | PASS |
| `LEAK.AOU` before/after | heap 667,024, 15 channels, 0 spaces, unchanged |
| R12 kernels rebuilt from this branch | byte-identical (`r12-pg-vga.img` `dd11fe87…`, `r12-pg.img` `1e00386b…`); default images: 6 × IDENTICAL |
