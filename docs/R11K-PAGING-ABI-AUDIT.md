# R11K paging ABI audit — what R11J built, what an ordinary C89 program meets

Branch `r11k-integration`, base frozen R11J `f7664d7`. This audit re-reads the R11J machinery
(`kernel/paging.{c,h,s}`, the hooks in `jobs.c`, `aout.c`, `imgload.c`, `channel.c`, `descriptors.c`,
`shellboot.c`) and the SDK start-up path (`sdk/lib/crt0.s`, `sdk/libc/*`) against the R11K goal: start two
byte-identical ordinary programs from the FAT16 volume, concurrently, in different address spaces.
Claim labels: **live-proven** (R11J transcripts, re-run in R11K Phase 0 baseline: SAMELNCH main PASS on
i386VM block and Tiny386), **code-reviewed**, **not tested**.

## 1. Launch path as it stands (`QT_PG_START`, 0x25)

`QT_PG_START(name, args, args_len)` from an **unpaged** caller:

1. `img_pg_start_request` validates the file name and argument bytes (`APP_ERR_ARGS` −34 if unterminated).
2. `app_load_paged`: reads the a.out header first (−20..−25 for bad magic/size/relocations/bss; −61
   `PG_ERR_ORIGIN` unless entry == 0x240000), allocates in this order — address-space record (−63), 1 MiB
   frame run from the pool (−60), job (−29) — and **rolls back every earlier step on any later failure**.
3. Zeroes the window through the frames' physical addresses, copies text+data, zeroes BSS, writes the
   argument block at the top of the window with `argv` pointers valid at the *virtual* addresses, builds
   the directory, binds record ↔ job, makes the job runnable.
4. Returns the tagged job id. Exit and release use the ordinary job services.

Live-proven in R11J (i386VM block ×3, Tiny386 ×3, QEMU ×3) and again in the R11K baseline run.

## 2. Shared vs private addresses

| Address range | In a paged process | Notes |
|---|---|---|
| 0x240000..0x33FFFF (1 MiB window) | **private**: its own frames, distinct physical pages per process, same virtual address | text, data, BSS, argument block, libc/libqdos statics (`stdout` buffer, `errno`-like state, `qd_entry_frame`) |
| kernel text/data/BSS, page-table pool, IDT/GDT | **shared**, identical mapping in every directory | supervisor PTEs by convention; CPL 0 means nothing is protected |
| kernel heap `[arena, 0x200000)` | **shared**, same virtual address everywhere | job stacks, `qd_alloc`/libc `malloc` blocks, channel objects |
| slot 0 (0x200000..0x23FFFF), transfer area, profiles 2/3 | shared (identity) on i386VM; on PC the profile 2/3 spans are the frame pool and are **unmapped** in a paged process | |
| frame pool | **unmapped** in every paged process (including its own frames except via the window) | observed fault CR2 0x900000 (i386VM) / 0x380000 (PC) |
| device page 0x800000.. (i386VM) | shared | console/timer/block registers |
| ≥ 0x1000000 | not present | any access faults |

Consequences for the R11K claim set: **image state is private; the heap, job stacks and the kernel are
shared and writable by every process** (ring 0). R11K does not change this.

## 3. Mechanisms re-audited

* **Template directory**: identity map 0–16 MiB, supervisor PTEs, built in `pg_init` before the volume and
  shell. Used by SYSTEM, IDLE, the shell and every unpaged job. **code-reviewed + live (identity paging runs
  the whole R11H/R11I regression suite).**
* **Per-process directory**: `pg_rec_build` shares template tables except those covering the window and the
  pool; 21 static page-aligned pages hold directories and private tables (3 records × 5 pages + template).
* **Frame allocator**: `app_frames_alloc` scans the pool with `region_reserve_at` for a contiguous 1 MiB run
  (contiguity is required on i386VM: its core uses only the first physical address of an access that crosses a
  page above 0x800000). i386VM block: 7 windows (0x900000..0xFFFFFF). PC/QEMU: 2 windows (0x380000..0x47FFFF
  and 0x500000..0x5FFFFF), i.e. the profile 2/3 spans — so on PC a paged process and a profile-2/3 program
  exclude each other (the region allocator refuses the second user). **live-proven (−60 on a third launch).**
* **Job switch**: every switch goes through `schedule()` with IF off. `pg_activate(job_current)` runs after
  the x87 restore and loads CR3 only when it differs. All job stacks are shared kernel heap, so the switch
  never loses its stack. Timer IRQ and cooperative yield both end in `schedule()`. **live-proven** (≈1300
  CR3 loads per SAMELNCH run; test image with `int 0x27` before and after every CR3 load).
* **Hold flags** (`QT_SUSPEND`, `QT_CON_WAIT`): the kernel writes the flag later, from another context. If the
  flag lies in the window or the pool and the owner's directory is not active, the write is deferred until
  right after the owner's directory is next activated. Applies to paged **and** unpaged owners (an unpaged
  slot-1 program's flag at 0x24xxxx is template memory). **live-proven**: 8/8 with the fix, 0/8 on the
  pre-fix R11J kernel (kept in `logs/r11j/controls`).
* **Poll/schedule-list tasks** (`QT_POLL_ADD`/`QT_SCHED_ADD`, unpaged registrants only): run from the timer
  ISR and scheduler under the template directory (`pg_enter_template`/`pg_leave_template`). **code-reviewed only
  in R11J**; there is no removal service, so a user-registered task cannot be removed. R11K adds a test-only
  *kernel-registered* task pair (a kernel function never goes away) to prove the template switch live;
  see `docs/R11K-RUNTIME-REPORT.md`.
* **User-pointer checks**: `chan_user_range_ok` for a paged caller accepts only its own window and the shared
  heap. **live-proven** (own 1, heap 1, 0x01000000 −14, pool base −14, slot 0 −14).
* **Services refused to paged callers**: `QT_CREATE`, `QT_POLL_ADD`, `QT_SCHED_ADD`, `QT_IMG_LOAD`,
  `QT_IMG_START`, `QT_PG_START` → −62. Rationale: each makes the kernel use the caller's private memory later in
  another address space. A paged program therefore **cannot start children**.
* **Exceptions**: vectors 0–19 have stubs (20–31 are reserved, gates not present). In R11J only a `#PF` whose
  EIP is in a paged job's window ends that job; everything else halts the machine (0x7E/0x7F). i386VM's CPU
  core ends the run at any guest exception before IDT delivery, so the handler is provable on Tiny386 and
  QEMU only.

## 4. What an ordinary program does (`crt0.s`, `libc`, `libqdos`)

* `crt0`: `QT_APP_ARGS` → argc/argv in the window; `main`; `QT_EXIT`. It does not clear BSS (the loader
  does). No absolute address other than the program's own link addresses; **no relocation records** and no
  profile-specific constant in `sdk/` (grep of all SDK sources; link scripts `slot0..3.ld` are the only place the
  origins appear). Header scan of the 49 programs on the R11J volume (`logs/r11k/audit/aout-scan.txt`): 45
  are OMAGIC, entry 0x240000, relocation size 0/0 (launchable as is); the other four are by design
  profile-bound (COLLIDE, MULTIA at 0x380000, MULTIB at 0x500000, SHELL at 0x200000) and are refused with −61.
* `libc` `malloc` = `qd_alloc` (kernel heap) + 16-byte header; blocks are owned by the job and reclaimed at
  exit; **shared address space, private ownership**. `stdout` is line-buffered over the `CON` channel; its
  buffer and the `FILE` table are libc statics in the window (private). `fopen`→`qd_open("WIN1_…")`→
  `QT_CH_OPEN`: the name is in the window (accepted); `fread`→`QT_CH_READ` into a window buffer (accepted).
* Console: output is shared (one console, writes interleave at line or write granularity). Input: a single
  waiter is allowed (`QT_CON_WAIT` returns −50 to a second one); characters go to whichever job reads next.
  There are no virtual consoles. R11K tests and documents this; it does not add consoles.
* `time`/ticks/`qd_self`: kernel services, no memory handed to the kernel beyond the stack/window.

## 5. Gaps found against the R11K goal, and how each ended

Status words are those of `docs/R11K-RUNTIME-REPORT.md`: *live-proven* (run in the matrix, judged by `scripts/r11k-assert.py`),
*code-reviewed*, *not tested*.

| # | Gap | Resolution | Status |
|---|---|---|---|
| 1 | No library entry point or generic launcher (R11J used raw `qtrap`). | `qd_pg_start`, `qd_pg_startv`, `qd_pg_cancel`, `qd_pginfo`, `qd_pg_caps`, `qd_pg_available` in a **separate libqdos member** (`qdospg.o`), so existing programs link byte-identically; launcher `PGRUN.AOU`. | live-proven (i386VM block, Tiny386, QEMU) |
| 2 | Flag name `QDOS_PAGING_POC` in a supported path. | `QDOS_PAGING` (ABI version 2); the old name is an `#error`. | build-verified |
| 3 | A fault other than `#PF` in a paged program's window halted the machine. | Any exception 0–19 at an EIP inside a paged job's window ends **that job** (status −17), provided that job's own directory is the active CR3 (review fix: otherwise the machine halts, so an innocent paged copy is never blamed for an unpaged task's fault). | live-proven on Tiny386 and QEMU (`TWICET F`, `Y`); not provable on i386VM (core limit) |
| 4 | No capability probe. | `QT_PG_CAPS` (0x27); SDK maps a non-paging kernel to `QD_PG_ERR_NOPAGING` (−70). | live-proven (`TWICET X`, nopaging sequence) |
| 5 | `QT_PG_INFO` exposes physical frame bases. | Kept for diagnostics; the SDK `qd_pginfo` exposes no physical frame address. | by design |
| 6 | Poll/schedule-list CR3 switch only code-reviewed. | Test-only kernel-registered tasks (`QDOS_TEST_PG_TASKS`) and an interrupt-around-CR3 hook; `TWICET Y` for the fault-in-task case. | live-proven (i386VM, Tiny386) |
| 7 | Overlap of two unmodified copies only probable. | `QD_PG_HELD`: each copy is created inactive and all are activated together (`qd_job_activate`), or removed with `qd_pg_cancel`. Group start is all-or-nothing. | live-proven; short programs may still end before they can be sampled live (reported NA, no claim) |
| 8 | Boot-size ceiling. | `platform/pc/boot2.s` + `make-image2.py` (`docs/R11K-BOOT-AUDIT.md`, `docs/R11K-BOOT-ABI.md`). | live-proven (Tiny386, QEMU) |

## 6. Capacity limits that stay

`JOB_MAX` = 6 (SYSTEM, IDLE, shell, launcher, two paged copies); `PG_PROC_MAX` = 3 records; frame windows 7
(i386VM block) or 2 (PC/QEMU). No change: the larger numbers are not independently tested.
