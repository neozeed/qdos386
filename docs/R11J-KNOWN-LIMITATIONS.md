# R11J — Known limitations

R11J is a proof of concept.  These are deliberate limits, emulator facts and open items.  None of them
is hidden by the runtime report.

## Not provided (by design)

* **No memory protection or isolation.**  Ring 0 only.  U/S bits are set as a convention but are
  not checked at CPL 0.  A paged process can write kernel memory, the shell, other jobs' heap blocks
  and the page tables (all are mapped), and could load CR3 itself.  The POC proves separate *mappings*,
  not a security boundary.  No ring-3, TSS, user stacks, demand paging, swap or PAE.
* **One window.**  Only slot-1 images (entry 0x240000, ≤ 1 MiB) can be paged.  Profiles 2/3 and slot 0
  are not paged.  The R11H fixed-profile loader is unchanged and remains the production path.
* **Heap is shared.**  `qd_alloc`/`QT_MEM_ALLOC` blocks and job stacks are global kernel-heap
  addresses, visible in every directory.  The per-process state proven private is the window: text,
  data, BSS and the argument block.
* **Services refused to a paged process**: `QT_CREATE`, `QT_POLL_ADD`, `QT_SCHED_ADD`, `QT_IMG_LOAD`,
  `QT_IMG_START`, `QT_PG_START` (`PG_ERR_REFUSED`).  A paged process cannot start other programs.
* **Fixed capacity**: 3 address-space records (`PG_PROC_MAX`); `JOB_MAX` (6) is unchanged, so with
  SYSTEM, IDLE, the shell and a launcher only two paged processes run at once.  On i386VM block the
  third launch is refused for want of a **job slot** (−29) although frames remain; frame exhaustion
  (−60) is reached naturally on the PC (2 MiB pool) and on i386VM only with the TEST-only 2 MiB cap image.
* **The window is zeroed and copied with interrupts off** (as the R11H loader does), about 1 MiB of
  writes per launch.

## Emulator facts

* **i386VM: guest exceptions end the run.**  The historical i386VM CPU core prints
  `guest exception N at EIP` and calls `exit(1)` before IDT delivery (`i386.c` `cpu_step`).  The core is
  not modified (plan constraint).  So on i386VM:
  * the guest #PF handler is **not live-proven**; a fault ends the harness;
  * the fault test runs last in its own boot, and the optional harness patch
    `patches/i386vm-faultdump.patch` (machine.c only, enabled by `I386VM_FAULTDUMP=1`) prints CR0, CR2,
    CR3 and EIP from the core's public state API.  The error code is not exposed by the core and is
    not reported;
  * the handler's report-and-continue behaviour is live-proven on Tiny386 PC and QEMU.
* **i386VM: multi-byte accesses that cross a page boundary above 0x800000** reach the harness with the
  first physical address only (`load16/32`, `store16/32`).  R11J gives each process one physically
  contiguous frame run, so accesses inside the window are correct (observed: SAME's `gvals` lies at an
  odd address and crosses a page).  A single access straddling the window's end (virtual 0x33FFFF/0x340000)
  would read frame-run bytes instead of the transfer area; no program does this, and it is not tested.
* **i386VM: code in the frame pool is fetched byte by byte through the harness** (the core does not
  cache instruction fetches above 0x800000).  Correct; slower.
* **Tiny386 PC**: 8 MiB; no spare span, so the pool is the profile 2/3 spans; an R11H program that
  needs profile 2 or 3 gets `APP_ERR_ADDROCC` while a paged process holds it (allocator exclusion,
  correct).
* **QEMU evidence is the 80×25 VGA text screen** sampled through the monitor.  Lines longer than 80
  columns are cut, so a few fields (per-copy preemption counts in the `end` lines, the A/B CR3 at the
  end of a fault line) are not in the QEMU transcripts; `r11j-assert.py` marks those checks NA rather
  than PASS.  The programs' own PASS/FAIL verdicts and the fault CR2/EIP fields are on screen.

## Review findings and their status

An independent review of the code and logs (after the first matrix) found two kernel defects, both
in the case of an **unpaged** slot-1 program running beside paged ones, and weaknesses in the tests:

| Finding | Status |
|---|---|
| Hold flag of an unpaged job written under a paged directory (wrong memory; a fault on the PC if the flag is in profile 2/3) | fixed (`pg_hold_release` defers for any owner); regression check in SAMELNCH: 8/8 on the fixed kernel, **0/8 on the pre-fix kernel** (`logs/r11j/controls/`) |
| Poll/schedule-list tasks of unpaged jobs run under a paged directory | fixed (task lists run in the template); **code-reviewed only**: the kernel has no task-removal service, so a transient test program cannot register one safely |
| Refused directory release would still free the job slot | fixed (slot stays dead, retried); the guard is unreachable by construction and never fired |
| SAME's expected pattern came from window memory (`id`, argv), so shared frames could go unnoticed | fixed: expectations derive from the kernel-reported job id kept on the job stack; **negative control** (`-DQDOS_TEST_PG_SHARE_FRAMES`, every copy mapped to the first copy's frames) makes the proof FAIL |
| Pointer check used 0x900000 as "pool" on the PC | fixed: the copy uses the pool base reported by `QT_PG_INFO` |
| Log checker relied on program verdicts for several items | strengthened: pointer-check values, deferred writes, unpaged hold flag, QEMU N/R content, negative control |
| Report errors (addresses, swapped counts, a QEMU MULTI/NEG claim without a run) | corrected; QEMU now runs MULTI; NEG is not claimed on QEMU |

## Other test caveats

* i386VM is deterministic: its three rounds replay identically (same counts).  They show
  reproducibility, not independent samples; Tiny386 and QEMU rounds vary.
* The interrupt-around-CR3 test uses a synchronous `int 0x27` with nothing pending.  It proves the IDT
  path, stubs and stacks work under both directories at exactly those points; asynchronous timer
  interrupts around switches are covered only statistically (≈1,300 switches per run).
* PC boot limit: kernel + transfer area must fit 0xF000 bytes (120 sectors).  The VGA POC kernel uses
  all 120 (59,256 + 1,648 bytes); exception stubs for reserved vectors 20–31 were dropped and messages
  shortened to fit.  Any growth of the VGA POC kernel needs a boot-loader change.

## Test-only builds (never release images)

`QDOS_TEST_FAULT_LAUNCH=3` (the first paged copy fails mid-copy), `QDOS_TEST_PG_POOL_BYTES=0x200000`
(2 MiB pool on i386VM), `QDOS_TEST_PG_INT_AROUND_CR3` (int 0x27 immediately before and after every CR3
load), `QDOS_TEST_PG_SHARE_FRAMES` (negative control: copies share frames; must FAIL).  Built by `scripts/r11j-images.sh` into `build/r11j/test-*`.

## Inherited, unchanged

* The three known `floatundidf` LGTEST discrepancies (R11C/R11D): not reopened.
* The R11F application-size difference: the rebuilt R11F applications are 38–54 bytes longer than the
  frozen ones.  R11J therefore builds its volume from the frozen R11H volume's own file bytes
  (`tools/r11j-extract-volume.py`, reproduction of `672806ad…` checked) plus SAME.AOU and SAMELNCH.AOU.
* Zork I: not run; the private story volume is not in this worktree and is not embedded.
* The FAT 8.3 rule: the plan's `SAME-LAUNCH.AOU` is `SAMELNCH.AOU`.
* R11I's `scripts/r11i-matrix.sh` names a legacy image the R11I package does not contain (see the
  audit); R11I is frozen, so the defect is documented and worked around, not fixed in R11I.
