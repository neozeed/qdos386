# QDOS/386 R8 - loader design

Status: implemented on branch `r8`. Verified on Tiny386 and i386VM (see
`R8-RUNTIME-REPORT.md`). Source: `kernel/aout.c`, `kernel/aout.h`,
`kernel/imgload.c`, `kernel/imgload.h`, `kernel/jobs.c`.

## 1. Format

The loader accepts only the historical i386 a.out OMAGIC format (magic 0407),
as produced by GCC 1.40 / GAS 1.38 / i386-ld 1.94 with `-N -T slotN.ld -e _start`.
There is no ELF, no relocation (relocation records are refused with
`APP_ERR_RELOC`), no dynamic linking and no second wrapper format.

## 2. Two sources, one policy

The loader is written against a reader callback:

    typedef int (*app_rd_fn)(void *ctx, unsigned int off, unsigned char *dst, unsigned int n);

Two readers exist and use the same validation:

- memory images (`mem_rd`, used by the R5/R6 transfer area and `app_load_bytes`);
- FAT16 files (`fat_rd` in `imgload.c`, over `fat16_seek` / `fat16_read`).

`app_load_src(rd, ctx, len, want_slot, packed, plen)` is the single entry point.
`aout_check_src` is the pure validation pass; it writes nothing.

## 3. Order of operations

    1. validate   aout_check_src: header, exact file length, layout, symbols, _start
    2. slot       want_slot must match the image's link address
    3. arguments  args_check: count, NUL termination, total size
    4. busy       slot owner must be free
    5. job        job_alloc_sized(prio 0 = inactive, stack APP_STACK_BYTES)
    6. reserve    slot_owner[slot] = APP_RESERVED
    7. copy       text, then data, in 512-byte chunks; BSS zeroed
    8. arguments  argc, argv pointer, strings written at the top of the slot
    9. commit     entry set, prio_inc 2, slot owner = job id

Steps 1-4 change no memory.  Step 5 happens before step 6, so a failed job
allocation (no free job slot, or no heap for the stack) leaves the slot exactly
as it was.  If the copy (step 7) fails, `job_cancel` frees the job and the slot is
released; no job, channel or heap block is left behind.

Caveat, stated plainly: after step 6 the free slot's bytes are overwritten.  That
slot is not running anything at that point, so no live job is affected, but a
failure during the copy leaves stale bytes in a free slot.

## 4. Start-symbol check (performance fix)

The loader must find `_start` with value equal to the entry point.  The first
version read each symbol record and then its name from the string table at a
random offset.  On a FAT16 file, a backward seek restarts the cluster-chain walk
from the first cluster, so the scan cost grew with symbols times clusters.

For `LARGE.AOU` (106837 bytes, 1413 symbols, 209 clusters) one load took about
12 s of emulator time and exceeded the harness's per-run budget of 10^9
instructions when two loads ran in one session.  The fix:

- symbol records are read forward, in chunks of 64 (`START_CHUNK`);
- a name is read only for a symbol whose value equals the entry point;
- at most 64 such candidates are examined (`START_CANDIDATES`), beyond which
  the load is refused with `APP_ERR_FORMAT`;
- outcome is unchanged: `APP_OK` if `_start` with the entry value exists,
  `APP_ERR_ENTRY` otherwise.

After the fix the same two loads complete in under a second, and the regression
suite and the R8 sessions pass.

## 5. Limits

| Limit | Value | Where |
|---|---|---|
| text + data + BSS | 229376 bytes (224 KiB), `APP_IMAGE_LIMIT` | `aout.h` |
| file length | 512 KiB, `IMG_FILE_MAX` | `imgload.h` |
| slot size | 256 KiB (unchanged) | `slot0.ld`, `slot1.ld` |
| argument strings | 16 including argv[0], 432 bytes including the name | `aout.h` |
| RUN line | 80 characters, `SH_LINE_MAX` (shell) | `shell.c` |
| application stack | 16 KiB from the kernel heap, `APP_STACK_BYTES` | `aout.h` |
| start-symbol candidates | 64 | `aout.c` |

The 432-byte argument limit cannot be reached from the shell's 80-character line
(the name and quotes use part of the line).  It is enforced by the loader and by
the shell parser; reaching it needs a direct `QT_IMG_LOAD` call.

## 6. Memory accounting for LARGE.AOU

| Quantity | Bytes | Notes |
|---|---|---|
| on-disk file | 106837 | 209 sectors; one sector per cluster on the R8 volume |
| header | 32 | |
| text | 48288 | read from the file at load |
| data | 32768 | read from the file at load |
| BSS | 98320 | zeroed by the loader, not read |
| symbol table | 16956 | read only for the start-symbol scan |
| string table | 8793 | read only for the candidate names |
| text+data+BSS | 179376 | 78% of the 229376 limit |
| stack | 16384 | kernel heap, not in the slot |
| application heap | shared kernel heap | `QT_MEM_ALLOC`; not per application, not in the slot |

Initialized load bytes are text plus data (81056).  The application heap is the
kernel's heap, which is shared and is not inside the slot; the R8 test program
allocates and frees a 4 KiB block and checks that the free count returns to its
starting value.

## 7. Refusals and what they leave behind

Verified by `tests/test_r8.c` (kernel, on both targets) and by the R8 session on
the R8 volume:

| Case | Result | Slot | Job / heap |
|---|---|---|---|
| read failure in validation | its error code | free | none left |
| read failure in the copy | its error code | released | job cancelled |
| file one byte longer / shorter | `APP_ERR_SIZE` | free | none |
| slot 0 image asked for slot 1 | `APP_ERR_NOTSLOT1` | free | none |
| text+data+BSS over 224 KiB | `APP_ERR_LAYOUT` | free | none |
| argument string not NUL-terminated | `APP_ERR_ARGS` | free | none |
| argument block over 432 bytes | `APP_ERR_ARGS` | free | none |
| seventeen argument strings | `APP_ERR_ARGS` | free | none |
| truncated / long / bad entry / bad magic files | -20 / -20 / -25 / -21 | free | none |
| busy slot | `APP_ERR_SLOTBUSY` | unchanged | none |
| no free job slot | `APP_ERR_NOJOB` | unchanged | none |
| no heap for the stack | `APP_ERR_NOMEM` | unchanged | none |

The volume is read only.  The loader never writes to WIN1_.

## 8. Not covered

- Exact-boundary acceptance at 224 KiB is covered by the kernel test
  (`tests/test_r8.c`), not by a file; the file-based negative is one byte over
  in the BSS field.
- Concurrent loads from two jobs: the kernel runs loads with interrupts masked,
  so a second load cannot interleave with the first; the busy-slot check covers
  the observable case.
- Wall-clock load time is not reported; the timing above is emulator time.
