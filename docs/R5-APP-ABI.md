# QDOS/386 R5 - native application ABI (Phase 1)

Status: Phase 1 decisions, now implemented. Sections 8 to 10 describe what
exists in the R5 tree. Runtime results are in `docs/R5-RUNTIME-REPORT.md`.

## 1. Memory map

Source of truth: `kernel/appmap.h`. Both targets have 8 MiB of RAM.

| Range | Use |
|---|---|
| `0x000000` .. kernel BSS end | kernel image. The i386VM harness links at 0; the PC kernel links at `0x100000`. |
| kernel BSS end .. `0x200000` | kernel heap (`HEAP_END = APP_SLOT0_BASE`). Job stacks, channel objects, user blocks. About 1 MiB on the PC, about 2 MiB on i386VM. |
| `0x200000` .. `0x23FFFF` | application slot 0 (256 KiB). Origin `0x200000`. |
| `0x240000` .. `0x27FFFF` | application slot 1 (256 KiB). Origin `0x240000`. |
| `0x280000` .. `0x2BFFFF` | transfer area (`XFER_BASE`, 256 KiB): directory and a.out bytes, written by the transport before the kernel starts. |
| `0x2C0000` .. `0x5FFFFF` | reserved, not in the heap. |
| `0x600000` .. `0x7FFFFF` | boot stack and firmware data (unchanged from R4). |

Consequences:

- The heap no longer reaches `0x600000`. It ends where application slot 0
  begins. R2 to R4 regressions pass on both targets with this bound (one run each in
  Phase 1; logs in `logs/r5-phase1/`).
- User pointers used by channel services are range-checked to
  `0 .. XFER_BASE + XFER_SIZE` (`USER_RANGE_END`). That covers the kernel, the
  heap, both application slots and the transfer area. It is still a range
  check, not a protection check (R3 policy). Applications run in ring 0 with a
  flat address space.

## 2. Linking an application

The unchanged `i386-gcc` driver always links the kernel runtime (`crt0.s`,
`i386.ld`, origin 0). It cannot produce an application image. Applications
are therefore compiled with the unchanged `i386-gcc -c` and `as`, and linked
with the unchanged `i386-ld` against one of the application-only scripts:

```
i386-gcc -c app.c -o app.o
as -o app_start.o apps/common/app_start.s
as -o qtrap.o apps/common/qtrap.s
i386-ld -N -T apps/common/slot0.ld -e _start -o app.aout app_start.o app.o qtrap.o
```

- `apps/common/slot0.ld` sets the text origin to `0x200000`. `slot1.ld` sets
  it to `0x240000`. Each script aligns data and BSS to 16 bytes, so the data
  and BSS addresses follow from the origin (see `docs/R5-AOUT-FORMAT.md`).
- An application is linked for exactly one slot. Its header `a_entry` must
  equal that slot's base, and the loader rejects anything else.
- The text must not be larger than the slot. The loader checks that the whole
  image (text, data and BSS) fits inside the slot.
- Relocations are not supported. The link produces none, and the loader
  rejects a file that has any.

## 3. Entry point and startup contract

`_start` (`apps/common/app_start.s`) is the entry. The kernel calls it as an
ordinary function from `job_run`, on the job's own stack.

- The loader has already copied text and data to the slot, and zeroed BSS.
  `_start` does not clear BSS, does not reset ESP and does not halt.
- `_start` calls `_main` with no arguments. `_main` returns an `int`.
- `_start` passes the return value to `QT_EXIT` and then returns. The kernel
  then ends the job (`job_exit`). The exit status is recorded even if the job
  is reclaimed later.

## 4. Stack

- Each job runs on a 4096-byte stack taken from the kernel heap
  (`JOB_STACK_BYTES`). The lowest 16 bytes hold the stack canary. Usable depth
  is about 4080 bytes.
- The stack is independent per job. A canary failure is caught by the existing R3
  canary checks.
- Applications must keep their frames small. There is no guard page in R5.

## 5. Globals and BSS

- Initialised data follows the text in the file. BSS follows the data in
  memory and is not in the file. The loader zeroes it before the job starts.
- The application must not assume any BSS symbol is at a fixed address other
  than the one the link script gives.

## 6. Traps used by applications

Calling convention unchanged from R3 and R4: `int $0x30`, EAX = function,
EBX/ECX/EDX = arguments, EAX = result. `qtrap()` in `apps/common/qtrap.s`
wraps it for C.

| Code | Name | Use in R5 |
|---|---|---|
| `0x14`–`0x17` | `QT_CH_OPEN`, `QT_CH_CLOSE`, `QT_CH_WRITE`, `QT_CH_PUTC` | console output (R4, unchanged) |
| `0x18` | `QT_EXIT` (new in R5) | a = exit status. Recorded for the calling job. Returns 0. The job ends when its entry returns. |
| `0x19` | `QT_JOB_EXIT` (new in R5) | a = job id. Returns the exit status, or a negative code below. |

Existing numbers `0x08`–`0x17` are unchanged.

`QT_EXIT` does not end the job by itself. The trap returns to the caller, and
`_start` returns after it. An application that returns from `_start` without
calling `QT_EXIT` ends with no status.

### 6.1 `QT_JOB_EXIT` results

| Result | Meaning |
|---|---|
| status (0 or more) | the job called `QT_EXIT` with that value |
| `-2` | no such job: the id was never created, or its slot has been reused by a later job (the same code QT_INFO uses) |
| `-16` | `JOB_ERR_RUNNING`: the job exists and has not ended |
| `-17` | `JOB_ERR_NOSTATUS`: the job ended without calling `QT_EXIT` |

The exit record is kept per slot (`exit_rec` in `kernel/jobs.c`). It survives
reclaim and is replaced when the slot is reused. A stale id therefore returns
`-2` once its slot has been reused, and it keeps its status until then.

## 7. Heap and channel ownership

- Each job owns the heap blocks it allocates. Its channels are closed, and its
  blocks are freed, when it is reclaimed (`reclaim_dead`, unchanged from R4).
- The loader's copies of text and data are not heap blocks. They live in the
  application slot, and the slot is released when the job is reclaimed. The
  release rule is part of Phase 2.

## 8. Transfer area and application loading (implemented)

The transfer area at `XFER_BASE` (`0x280000`, 256 KiB) carries the executables.
Both targets present the same bytes at the same address, and both use the same
guest loader (`kernel/aout.c`).

### 8.1 Layout (`tools/make-xfer.py`)

| Offset | Content |
|---|---|
| `0x00` | directory: u32 magic `QXFR` (`0x52465851`), u32 version 1, u32 count (1..4), u32 0 |
| `0x10` | entries, 16 bytes each: u32 offset (from `XFER_BASE`), u32 exact length, 4-byte name, u32 0 |
| `0x80` | first payload; each payload starts on a 16-byte boundary |

The directory length is the exact file length. Padding after a payload is never
read as part of the file. The loader rejects a directory whose entries overlap,
start inside the directory, lie outside the area, or number more than four.

Current contents: `HELO` (hello, slot 0), `PREP` (prep, slot 1), `HEL2` (a second
copy of hello, used only to test the busy-slot refusal).

### 8.2 How the bytes arrive

- **i386VM.** `tools/make-vm-image.py` writes the kernel's raw image, zero-pads it
  to `0x280000` and appends the transfer area. The harness loads the file at guest
  address 0, so the payload is at `0x280000` with no copying. The zero padding is
  never interpreted. This image is 2.6 MB. The approved zero-padding approach,
  not a harness change.
- **Tiny386 PC.** `platform/pc/make-image.py` puts the transfer area in sectors after
  the kernel. The boot sector reads kernel and transfer sectors in one INT 13h call
  into the real-mode buffer at `0x10000` (about 56 + 4 sectors, within the 64 KiB
  limit and the 127-sector call limit). Protected-mode code copies the kernel to
  `0x100000`, then copies the transfer sectors from the buffer to `0x280000`. The
  copy length is the sector count from the boot sector, and the directory gives the
  true lengths.

### 8.3 Startup and BSS safety

- Kernel BSS ends below `0x200000`, far from `0x280000`. The check
  "kernel BSS ends below the transfer area" verifies this at run time.
- The heap ends at `0x200000`, so no heap block can cover the transfer area or the
  application slots.
- The run checks that the transfer area is byte-identical at the end of the
  application phase ("transfer area unchanged through the application phase").

## 9. The loader (implemented)

`aout_check` validates an image without writing memory. `app_load` runs it, then:

1. refuses if the slot is in use (`APP_ERR_SLOTBUSY`);
2. with interrupts off, creates the job with a placeholder entry and inactive
   priority (`job_alloc`), which may fail with `APP_ERR_NOJOB` or `APP_ERR_NOMEM`;
3. copies text and data into the slot and zeroes BSS;
4. sets the real entry and activates the job.

Because the job is created first, a refused load leaves the slot untouched. The
copy happens before the job can run. Reclaim releases the slot
(`app_release_owner`, called from `reclaim_dead`).

Checks, in order (each has a test with a mutated heap copy of `hello`):
magic (`APP_ERR_MAGIC`); relocation counts zero (`APP_ERR_RELOC`); entry equal to a
slot base (`APP_ERR_ENTRY`); text, data, BSS and symbol-table sizes aligned and
within the slot (`APP_ERR_LAYOUT`, `APP_ERR_FORMAT`); sections and tables inside the
file without overflow (`APP_ERR_SIZE`); string table size equal to the bytes that
remain, so truncation and trailing bytes are both refused (`APP_ERR_SIZE`); and a
`_start` symbol whose value equals the entry (`APP_ERR_ENTRY`).

## 10. Applications

- `apps/hello/hello.c`: opens `CON`, writes one line through `QT_CH_WRITE`, closes,
  and returns 7.
- `apps/prep/prep.c`: checks initialised data, zero BSS, eight-level recursion
  with a 64-byte frame at each level, and a 2,000,000-iteration volatile loop that
  never yields. Writes one line through the channel and returns 42 (or the mask of
  failed checks).

Both are linked with `i386-ld -N` against `apps/common/slot0.ld` or `slot1.ld`
and started through `apps/common/app_start.s`.

## 11. Limits of R5

- Two application slots, 256 KiB each. Applications are linked for a slot and
  cannot move.
- Ring 0, flat address space. An application can read or write any memory the
  kernel can. The channel range check is not a protection check.
- One console driver. No file, keyboard or serial I/O.
- The loader accepts only OMAGIC `0407` images with no relocation records.
  ZMAGIC, QMAGIC, shared libraries and relocation are not supported.
- The transfer area holds at most four images, in 256 KiB.
