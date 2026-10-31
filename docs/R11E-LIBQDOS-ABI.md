# R11E libqdos ABI

`libqdos.a` is the native QDOS/386 service library. It is separate from `libc.a`,
`libm.a` and `libgcc-qd.a`. Header: `sdk/include/qdos386.h`. Sources: `sdk/lib/qdos.c`
(wrappers), `sdk/lib/qtrap.s` (trap entry and exit unwind), `sdk/lib/crt0.s` and
`sdk/lib/crt0_libc.s` (startup). The i386 calling convention is the historical GCC 1.40
cdecl convention; EBX is saved by `qtrap`, as it is callee-saved there.

## Trap entry

`int qtrap(unsigned fn, unsigned a, unsigned b, unsigned c)` loads EAX = fn, EBX = a,
ECX = b, EDX = c, executes `int $0x30` and returns EAX. Every wrapper is a thin call to
`qtrap`. Pointers passed in `a`, `b` or `c` must lie in the user range; the kernel refuses
others with `CHAN_ERR_RANGE` (-14), so no wrapper reads or writes kernel memory.

## Wrappers

| Function | Trap | Result |
|---|---|---|
| `void qd_exit(int status)` | 0x18 + unwind | records status, ends the job; does not return |
| `void qd_exit_record(int status)` | 0x18 | records status only (crt0 main-return path) |
| `int qd_args(char ***argv)` | 0x1F | argc; `*argv` is the argument vector |
| `unsigned long qd_ticks(void)` | 0x20 | tick counter, low 32 bits |
| `void qd_yield(unsigned int ticks)` | 0x08 | timed suspend of the calling job |
| `void *qd_alloc(unsigned int bytes)` | 0x10 | block or NULL |
| `int qd_alloc_ex(unsigned int bytes, void **out)` | 0x10 | 0 with `*out` set; a `MEM_ERR_*` code; -3 (no memory) if the kernel returns NULL |
| `int qd_free(void *block)` | 0x11 | 0, or negative |
| `unsigned int qd_free_bytes(void)` | 0x12 | free heap bytes |
| `int qd_heap_ok(void)` | 0x13 | 1 if the heap invariants hold |
| `int qd_mem_validate(void)` | 0x13 | 0 if consistent |
| `int qd_open(const char *name)` | 0x14 | channel id > 0, or negative |
| `int qd_close(int ch)` | 0x15 | 0, or negative |
| `int qd_write(int ch, const void *buf, unsigned int n)` | 0x16 | bytes written, or negative |
| `int qd_putc(int ch, int c)` | 0x17 | the driver's count (1 for a byte), or negative |
| `int qd_read(int ch, void *buf, unsigned int n)` | 0x1A | bytes read; 0 at end; negative on error |
| `int qd_seek(int ch, unsigned long pos)` | 0x1B | 0, or negative |
| `int qd_getc(int ch)` | 0x1A | 0..255, `QD_EOF` (-1) at end, or the error |
| `int qd_puts(const char *s)` | 0x14, 0x16 | opens `CON`, writes the string; bytes written, or the negative open/write error |
| `int qd_self(void)` | 0x21 | tagged id of the calling job |
| `int qd_job_slot(unsigned int slot)` | 0x22 | tagged id in that slot, or `QD_ERR_NOJOB` (-2) |
| `int qd_job_info(int id, unsigned int *inc, unsigned int *acc)` | 0x0C | 0, or an error; `inc` and `acc` filled |
| `int qd_job_activate(int id, unsigned int inc)` | 0x0A | 0; -1 if already active |
| `int qd_job_chprio(int id, unsigned int inc)` | 0x0B | 0 |
| `int qd_job_reactivate(int id)` | 0x09 | 0; releases a suspended job |
| `int qd_job_suspend(long ticks)` | 0x08 | the calling job; `ticks < 0` = until released |
| `int qd_job_exit_status(int id)` | 0x19 | status, or a `QD_ERR_*` code |
| `int qd_job_reclaimed(int id)` | 0x1E | 1 if the job's slot is free |
| `int qd_run(const char *name, const char *args)` | 0x1D | job id, or error; `args` may be NULL |
| `int qd_con_wait(unsigned char *flag)` | 0x1C | sleeps until console input |
| `int qd_dir_open(void)` | 0x14 | channel on `WIN1_` (the root directory) |
| `int qd_dir_next(int ch, struct qd_dirent *e)` | 0x1A | 1 with an entry, 0 at end, or negative |

`qd_dirent` is `{ char name[13]; unsigned long size; }`. `qd_dir_next` copies the 12-byte
name, NUL-terminates it and decodes the size from the 16-byte directory record.

## Error codes

| Code | Name | Meaning |
|---|---|---|
| -2 | `QD_ERR_NOJOB` | stale or unknown tagged job id |
| -6 | `CHAN_ERR_NOSLOT` | no free channel |
| -7 | `CHAN_ERR_NAME` | no driver with that name |
| -12 | `CHAN_ERR_BADID` | invalid, closed or stale channel id |
| -13 | `CHAN_ERR_OWNER` | channel belongs to another job |
| -14 | `CHAN_ERR_RANGE` | buffer outside the user range, or too long |
| -15 | `CHAN_ERR_NOTSUP` | operation not supported by the driver |
| -16 | `QD_ERR_RUNNING` | job still running (exit status not yet recorded) |
| -17 | `QD_ERR_NOSTATUS` | no exit status recorded |
| -18 | `CHAN_ERR_NOVOL` | `WIN1_` volume not mounted |
| -19 | `CHAN_ERR_AGAIN` | console has no input yet; wait with `qd_con_wait`, then retry |
| -28 | `APP_ERR_SLOTBUSY` | the child slot is occupied |
| -50 | `QD_ERR_CONBUSY` | another job already waits for console input |

Status values -2, -16 and -17 are also ordinary exit statuses. A program that exits with
one of them cannot be told apart from the error from `qd_job_exit_status`. See
`docs/R11E-KNOWN-LIMITATIONS.md`.

## Names

`CHAN_NAME_MAX` is 18 (17 characters plus NUL), so a `WIN1_` name carries a full 8.3 file
name: at most 12 characters including the dot. `IMG_NAME_MAX` is 18 and `SH_NAME_MAX` is
12 for the shell. `FILENAME_MAX` in `stdio.h` is 18. A 13-character name is refused: `qd_open`
returns a negative error and `fopen` returns NULL. Old short names are unchanged.

## Kernel changes in R11E

- `QT_JOB_SELF` (0x21): returns the tagged id of the calling job.
- `QT_JOB_SLOT` (0x22): `a` = slot index, returns its tagged id, or -2 if the slot is free.
- `QT_SUSPEND` and `QT_CON_WAIT`: the hold pointer (`b`) is checked with
  `chan_user_range_ok(b, 1)` before use, because the kernel writes it later.
- `chan_user_range_ok`: the lower bound is now the heap arena base (`mem_arena_base()`),
  so a user buffer cannot name the kernel image or BSS. Before R11E only the upper bound
  was checked. The upper bound is unchanged (`USER_RANGE_END`).
- `mem_arena_base()` is a new query in `memory.c`.

No existing trap number changed. The two new traps are the only additions to the trap
table.

## Exit and unwind

`exit(status)` and `qd_exit(status)` record the status with `QT_EXIT`, then unwind to the
job's entry frame. `crt0.s` and `crt0_libc.s` store the frame pointer in
`_qd_entry_frame` at `_start`. `_qd_end_job` (in `qtrap.s`) restores that frame and
returns from `_start`, which ends the job exactly as `main`'s return does. A program
therefore never has to return from `main` after calling `exit`, and the shell recovers.

If no entry frame was recorded, `qd_exit` falls back to a suspend loop. This cannot occur
for programs linked with either crt0.

Before R11E, `qd_exit` looped on `QT_SUSPEND(-1)` and the job never ended; the shell
waited forever. That is the defect R11E fixed (CHILD2 test).

## Priority

`qd_job_chprio(id, 0)` makes the job inactive: it is no longer scheduled. Do not use an
increment of zero on a running job. `qd_job_activate` requires a non-zero increment.
