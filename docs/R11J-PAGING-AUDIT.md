# R11J Phase 0 — Paging audit (memory, execution model, emulator support)

Branch `r11j-paging`, from frozen R11I `bc9a90f` (archive SHA256
`fb7b341495d0cd1fe6ec75649b163733010f82e7e5164f11259777b871539924`).  R11H-FIX1 and R11I are not
modified.  All line numbers below refer to the files named; the emulator line numbers refer to
`emulator-tiny386/i386.c` in the historical i386VM tree (the i386VM core) and to
`tiny386-master/i386.c` (upstream Tiny386, the PC emulator's source).

## 1. Frozen baseline

| Item | Result |
|---|---|
| R11I archive, fresh extraction | manifest OK, 968 files |
| R11I harness rebuild | `i386vm-r9` `a4724f79b26a716e`, `i386vm-r9-block` `5970e16223075cf5` |
| R11I block image rebuild | `3ee37c19ac52acbc` (identical) |
| Baseline MULTI ×3 / NEG ×3 on i386VM legacy, i386VM block, Tiny386 | 9/9 MULTI PASS, 9/9 NEG 848/0 PASS, volumes unchanged |

**R11I packaging defect found (R11I stays frozen).**  `scripts/r11i-matrix.sh` starts the legacy MULTI
rounds from `build/r11i/legacy/qdos386-r11i-legacy.bin`, which the R11I package does not contain (it
was dropped as a duplicate of `build/i386vm-r11h-rel/qdos386-r11h-rel.bin`, same bytes `c5462138…`).
On a fresh extraction those three runs fail with "No such file" while the script still prints
`MATRIX DONE`, because it does not assert results.  The baseline above reran them from the shipped
image (3/3 PASS).  `scripts/r11j-matrix.sh` uses the shipped path and every result is judged by
`scripts/r11j-assert.py`.

## 2. CPU core paging support (tested, not assumed)

### i386VM core (historical tree)

| Behaviour | Source | Consequence |
|---|---|---|
| CR0.PG, CR3, 2-level 4 KiB walk, A/D bits | `tlb_refill` 570–605, `translate_lpgno` 607–640 | standard non-PAE paging works (live-proven) |
| CR3 load flushes the TLB; CR0 PG/WP/PE change flushes | `mov cr` 2286–2295 | no stale translations after a switch |
| INVLPG flushes the whole TLB | 3763 | not used by R11J |
| Table walk reads `phys_mem` directly, no bounds or I/O check | 586–597 | page tables must be real RAM below the core's RAM size: R11J keeps them in kernel BSS |
| Physical address ≥ 0x800000 goes to the harness callbacks | `in_iomem` 753 | frames in the R11I spare span (0x900000+) are served by `machine.c`, correctly |
| A 16/32-bit access crossing a page boundary on that path uses the **first** physical address only | `load16/32`, `store16/32` 770–860 | two non-adjacent frames would be read/written wrongly; R11J allocates each process **one physically contiguous run** (§5) |
| Instruction fetch from ≥ 0x800000 is never cached; byte by byte through the harness | `peek8` 887–909 | correct, slower |
| **Any guest exception ends the run before IDT delivery**: `fprintf(stderr,"guest exception %d at %08x")`, `exit(1)` | `cpu_step` 5146 | a guest #PF handler can never run on i386VM.  Not patched (the plan forbids changing the core) |
| `cpui386_get_state` exposes CR0/CR2/CR3/EIP | 5477 | optional harness patch `i386vm-faultdump.patch` (machine.c only) prints them at exit |

The core's exception number is public (`cpui386_get_excno`); the error code is not, so the i386VM
fault report has no error code.

### Upstream Tiny386 (the PC emulator)

* `in_iomem` = VGA window 0xA0000–0xBFFFF and ≥ 0xE0000000 only (753): all 8 MiB of PC RAM is on the
  ordinary path, so cross-page accesses are split correctly.
* Exceptions are delivered through the IDT with an error code for #DF/#TS/#NP/#SS/#GP/#PF
  (5139–5155); the emulator binaries of record contain no "guest exception" abort string.
* Live-proven: identity paging, per-process directories, and the guest #PF handler (§ runtime report).

### QEMU

Reference x86 paging.  Live-proven with the VGA kernel and scripted monitor keys.

## 3. Physical memory actually routed to RAM

| Target | RAM | Notes |
|---|---|---|
| i386VM (R9 harness) | 16 MiB: `ram` array 0..0xFFFFFF; 0x800000–0x800FFF device page; 0x801000–0xFFFFFF served from the same array by `machine.c` (R9 patch) | nothing exists above 16 MiB; R11J maps nothing there, so 0x01000000 always faults |
| Tiny386 PC | 8 MiB (`mem_size = 8M`), VGA text at 0xB8000 (I/O path) | PC map has no spare span; frames come from the profile 2/3 spans |
| QEMU | `-m 16` | the PC kernel uses the same PC map |

R11I's spare span 0x900000–0xFFFFFF is real RAM on i386VM (proven in R11I by writing and reading
every page) and is the i386VM frame pool.  It does not exist on the PC.

## 4. Kernel memory and stacks (R9 map; details in R11J-MEMORY-MAP.md)

* Kernel text/data/BSS: i386VM from 0, PC from 0x100000 (`platform/pc/entry.s` `kstart`).  POC kernel:
  text 57,088, data 576, BSS 140,288 bytes (i386VM); the BSS includes the 21-page page-table pool.
* Heap: from the aligned end of BSS to `HEAP_END` = 0x200000 (`shellboot.c`, `memory.h`).
* Job stacks: heap blocks (`job_alloc_sized`, jobs.c 309), 16 KiB for applications
  (`APP_STACK_BYTES`), with a canary checked on every switch-out.
* Boot (SYSTEM) stack: top 0x7FFFF0 (PC `entry.s` line 9; i386VM verified in
  `docs/R11H-FIX1-MEMORY-MAP.md`), in 0x600000–0x7FFFFF.
* x87 save areas: `jobs[n].fpu` (FNSAVE images) in kernel BSS.
* IDT, GDT: static arrays in `descriptors.c` (BSS).
* Transfer area: 0x340000–0x37FFFF (read at boot).

**Every stack and every structure the scheduler touches is in kernel memory below 0x200000 or in the
boot stack area, never in an application window or the frame pool.**  This is what makes the CR3
switch safe without a separate switch stack (§6).

## 5. Execution model and scheduler paths

All switching goes through `resume_dispatch` (`context.s` 45–60) with EAX = saved_sp | fmt.

| Path | Entry | Interrupts | Switch point |
|---|---|---|---|
| IRQ0 | `irq0_stub` → `sched_irq_tick` → `schedule(sp,1,1)` | gate clears IF | `schedule` |
| Cooperative yield | `yield_asm` (cli) → `sched_yield_sp` → `reclaim_dead`, `schedule(sp,0,0)` | off | `schedule` |
| Service trap | `trap_stub` → `trap_dispatch`; SUSPEND/REACTIVATE/CON_WAIT call `schedule(sp,1,0)`, others return `sp|1` | gate clears IF | `schedule` |
| IRQ1/IRQ7/IRQ15 | stubs, popal/iret | off | never schedule |
| Job creation | `job_alloc_sized` builds a format-0 frame returning to `job_trampoline` | caller's | first dispatch |
| Job exit | `job_exit`: IF off, dead, `yield_asm`; `schedule` sets `dead_mask` after saving the frame | off | `schedule` |
| Reclamation | `reclaim_dead` at the start of every trap and yield, for dead slots other than `job_current` | off | — |
| Nested critical sections | `irq_save`/`irq_restore` restore IF only if it was set | — | — |
| Return to the job | format 1: `popal; iret`; format 0: pops, `sti; ret` | — | — |

* **ESP/EIP/EFLAGS**: saved on the job's own stack (pushal + CPU frame, or callee-saved + return
  address); `saved_sp` in `struct job`.
* **FPU**: `fpu_save` (FNSAVE) of the outgoing job and `fpu_restore` (FRSTOR) of the incoming one in
  `schedule`, inside the `next != job_current` branch.
* **CR3**: none in the default kernel (one address space).  POC: `pg_activate(job_current)` right after
  `fpu_restore`, IF off, on every path above, since all of them switch in `schedule`.

## 6. Pointer validation and kernel writes into user memory

| Use | Source | Context | Paging hazard | POC handling |
|---|---|---|---|---|
| `chan_user_range_ok` | channel.c 95 | global interval [arena base, 0x600000) | same virtual address means different memory per process | paged caller: only its own window and the shared heap (`pg_user_range_ok`) |
| Channel read/write buffers | channel.c | the caller's own trap | none | — |
| `QT_APP_ARGS` writes `*a` | jobs.c | own trap | none | argument block is built at the **virtual** window top |
| **Hold flags** `*j->hold = 0xFF` | jobs.c `pick_next` (timer ISR), `con_input_arrived` (IRQ1), `QT_REACTIVATE` (another job's trap) | **any** context | written under the wrong CR3, for paged **and** unpaged owners | `pg_hold_release`: a flag in the window or pool is written only under its owner's directory, otherwise deferred (`pg_after_switch`) |
| `QT_POLL_ADD` / `QT_SCHED_ADD` nodes, `QT_CREATE` entry | jobs.c | run later in any context | registrant's code/data used under another directory | refused for paged callers (`PG_ERR_REFUSED`); task lists run in the template (unpaged registrants) |
| Loader copy and zeroing (`copy_src`) | aout.c 505 | caller's trap, IF off | the normal loader writes slot addresses under the caller's CR3 | `QT_IMG_LOAD`/`QT_IMG_START` refused for paged callers; the paged loader writes frames by physical address under the template |
| Exit records, job table, region pool | kernel BSS | any | none | — |

`QT_IMG_LOAD` (slot 1 by name) and `QT_IMG_START` (R11H profiles) are unchanged for unpaged callers.

## 7. a.out constraints and the common base

* `aout_check_src` refuses relocation records (`APP_ERR_RELOC`, aout.c 375) and requires the entry
  to be a profile base with a matching `_start` (`APP_ERR_ENTRY`, 378–421).  Every historical image is
  therefore absolute and fixed-origin.
* Common virtual base: **0x240000** (slot 1, 1 MiB on the R9 map), the SDK default (`qdos386-gcc -slot 1`).
  No collision: slot 0 (the shell) ends at 0x23FFFF and the transfer area starts at 0x340000.

## 8. Risks and decisions

| Risk | Decision |
|---|---|
| CR3 switch while running on a job-private stack | none exist: all stacks are shared kernel heap; switch in `schedule` with IF off |
| Kernel write into the wrong address space | hold flags deferred; later-use pointers refused; loader writes physical frames under the template |
| i386VM cross-page access on the I/O path | one contiguous frame run per process (observed: SAME's `gvals` crosses a page at an odd address and is correct) |
| i386VM aborts on guest exceptions | fault test runs last on i386VM, reported by the optional faultdump harness; the guest handler is proven on Tiny386 PC and QEMU |
| Page-table memory | static, page-aligned pool in kernel BSS (identity in every directory) |
| Default images change | every hook is under `QDOS_PAGING_POC`; the POC sources are added only through `I386EXTRA`/`PCEXTRA`; all default images re-verified byte-identical |

No blocker prevented the POC.  The i386VM exception abort limits what can be proven on i386VM (fault
continuation); it does not block the same-origin proof.
