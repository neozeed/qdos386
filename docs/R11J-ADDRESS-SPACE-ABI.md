# R11J — Address-space ABI (paging proof of concept)

**Status: proof of concept, opt-in.**  Only images built with `-DQDOS_PAGING_POC` (Makefile targets
`r11j-i386vm-poc`, `r11j-pc-poc`, or `scripts/r11j-images.sh`) contain any of this.  Default images
are byte-identical to R11I.  This is not memory protection: everything runs at CPL 0, and a paged
process can still reach kernel memory and change CR3.  It replaces nothing in the R11H loader.

## 1. Model

* **Paged process**: a job started by `QT_PG_START`.  It has its own page directory, in which the
  common window `0x240000..0x33FFFF` (slot 1, 1 MiB) maps a private, physically contiguous frame run.
  The whole frame pool is unmapped in it.  Everything else is the shared kernel template (identity).
* **Unpaged job**: SYSTEM, IDLE, the shell, and every image started by `QT_IMG_LOAD` (RUN) or
  `QT_IMG_START`.  They run on the template, exactly as in R11I.
* Two paged processes may be started from **the same file bytes**; both execute at the same virtual
  entry 0x240000.  An unpaged slot-1 program may run at physical 0x240000 at the same time (the
  launcher SAMELNCH does): every kernel access on its behalf made from another context uses the
  template (§6).

## 2. Image requirements

Unchanged a.out rules (OMAGIC 0407, no relocation records, `_start` at the entry), and the image must
be linked for **slot 1** (`qdos386-gcc -slot 1`, the SDK default; entry 0x240000).  Text + data + BSS
up to `APP_IMAGE_LIMIT_OF(1)` (1 MiB less the 512-byte argument block).  No SDK, linker-script or
toolchain change: the proof programs use the unchanged SDK and its public `qtrap`.

## 3. Services (int 0x30)

### `QT_PG_START` (0x25)

| Register | In | Out |
|---|---|---|
| EAX | 0x25 | tagged job id (> 0), or a negative code |
| EBX | file name (WIN1_ root, 8.3, NUL-terminated) | |
| ECX | argument bytes: NUL-terminated strings after the program name, or 0 | |
| EDX | argument length (0..`APP_ARG_STR_MAX` − name − 1) | |

The kernel validates the header first (reads only), then allocates in this order: address-space
record, frame run, job.  Each failure undoes what came before it.  On success the window is zeroed,
text and data copied, BSS zeroed, the argument block written at the top of the window with pointers
valid at the **virtual** addresses (`argv` as for RUN), and the job made runnable (increment 2).

Errors: the a.out codes (−20..−25), file errors, `APP_ERR_ARGS` (−34), `APP_ERR_NOJOB` (−29),
`APP_ERR_NOMEM` (−30), and:

| Code | Name | Meaning |
|---|---|---|
| −60 | `PG_ERR_NOFRAMES` | no contiguous 1 MiB run free in the pool |
| −61 | `PG_ERR_ORIGIN` | image not linked at 0x240000 |
| −62 | `PG_ERR_REFUSED` | caller is a paged process (see §5) |
| −63 | `PG_ERR_NOREC` | all `PG_PROC_MAX` (3) address-space records in use |

### `QT_PG_INFO` (0x26)

EBX = tagged job id, or 0 for the caller; ECX = address of an 18-word buffer.  Returns 0, or −2 for a
stale/unknown id or an unacceptable buffer.  Words:

| # | Field | # | Field |
|---|---|---|---|
| 0 | paged (0/1) | 9 | dispatches of that job |
| 1 | its directory (CR3 value) | 10 | live regions in the application pool |
| 2 | physical base of its frames (0 if unpaged) | 11 | bytes they hold |
| 3 | frame bytes | 12 | PTE that maps 0x240000 in its directory |
| 4 | window base (0x240000) | 13 | CR3 active during the call (the caller's) |
| 5 | window size | 14 | hold-flag writes deferred so far |
| 6 | frame bytes held by all paged processes | 15 | live address-space records |
| 7 | CR3 loads since paging was enabled | 16 | IRQ7 vector entries (test hook) |
| 8 | timer preemptions of that job | 17 | frame pool base |

## 4. What a paged process may pass to the kernel

`chan_user_range_ok` accepts, for a paged caller, only:

* its own window `0x240000..0x33FFFF`, and
* the shared kernel heap `[arena base, 0x200000)`: its job stack and its `QT_MEM_ALLOC`/`qd_alloc`
  blocks (**shared, global addresses**, labelled as such: the heap is not per-process).

Everything else is `CHAN_ERR_RANGE` (−14), including slot 0 (the shell), the profiles, the frame pool
and any address it does not map (observed: own 1, heap 1, 0x01000000 −14, 0x900000 −14, 0x200000 −14).
Unpaged callers keep the R11I rule.

## 5. Services refused to a paged process

`QT_CREATE`, `QT_POLL_ADD`, `QT_SCHED_ADD`, `QT_IMG_LOAD`, `QT_IMG_START` and `QT_PG_START` return
`PG_ERR_REFUSED`.  Each would make the kernel use the caller's private memory later, in another
address space (an entry point, a task node), or would copy an image under the caller's directory.
All other services are unchanged.

## 6. Kernel accesses made outside the owner's own context

* Done in the caller's own trap (its directory is active): channel reads, `QT_APP_ARGS`, `QT_PG_INFO`.
* **Hold flags** (`QT_SUSPEND`, `QT_CON_WAIT`): released by the timer ISR, IRQ1 or another job's trap.
  The window and the pool mean different memory in different directories, so a flag there is written
  only while **its owner's** directory is active, **paged or not** (an unpaged slot-1 program's flag at
  0x24xxxx belongs to the template).  Otherwise the write is deferred and made right after the owner's
  directory is next activated, before the owner runs; only the owner observes the flag.  Flags in the
  kernel heap are identical everywhere and written at once.  Observed: 24 deferred writes per SAME pair;
  the launcher's own slot-1 flag set 8 of 8 while paged copies run.
* **Poll and schedule-list tasks** (`QT_POLL_ADD`, `QT_SCHED_ADD`, registered only by unpaged jobs):
  run from the timer ISR and scheduler, so the kernel switches to the template directory around the
  task lists (when they are non-empty) and back.

## 7. Scheduling and exceptions

* CR3 is loaded in `schedule()` immediately after the incoming job is chosen and its x87 state
  restored, with interrupts off, only when it differs from the active one.  All stacks are shared kernel
  memory, so the switch code never loses its stack.
* Exception vectors 0–19 (POC images; 20–31 are reserved and stay not present): the handler prints
  vector, error code, CR2, EIP, job id and CR3.  A #PF whose EIP is in a paged process's window ends that process (its exit status is then
  `QD_ERR_NOSTATUS`, −17); any other exception halts the machine (status 0x7E; nested 0x7F).  On
  i386VM the CPU core ends the run before the handler can run (see the known limitations).

## 8. Lifetime

Frames and the directory belong to the tagged job id.  They are released by the ordinary reclamation
(`reclaim_dead`, from another job's trap or yield), never while the directory is active.  A stale id is
refused by `QT_PG_INFO` and every job service, as for any job.  Slot reuse gives a new tag.
