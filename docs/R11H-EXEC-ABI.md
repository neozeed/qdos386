# R11H executable ABI

Base: R11G `ffe2349`. Final R11H behaviour, verified by the tests in `docs/R11H-MULTITASKING-TESTS.md`.

## 1. What R11H does and does not do

R11H supports **several fixed-origin application profiles**. An executable is linked for one profile
base, which is its entry address. The loader accepts it only at that base, and runs it there. The loader
never moves an image.

R11H does **not** implement general executable relocation. The a.out format used here carries no
relocation records: every shipped image has `trsize = 0` and `drsize = 0`, and the loader refuses any
image that has them (`APP_ERR_RELOC`). Code and data addresses are fixed at link time. Moving an image
would need relocation data the format does not carry, or rewriting its code. Neither is in R11H.

R11H also does not implement paging, privilege rings, ELF, FAT writes, FAT32, virtual framebuffers,
SuperBASIC integration, or a shell redesign.

## 2. Profiles

| Profile | Base | Size (R9 map) | Size (default map) | Caller |
|---|---|---|---|---|
| 0 | 0x200000 | 256 KiB | 256 KiB | boot: shell |
| 1 | 0x240000 | 1 MiB | 256 KiB | `RUN` from the shell; `qd_start(name, 1)` |
| 2 | 0x380000 | 1 MiB | 0 (absent) | `qd_start(name, 2)` |
| 3 | 0x500000 | 1 MiB | 0 (absent) | `qd_start(name, 3)` |

`RUN name` (the shell's command, `QT_IMG_LOAD`) loads only profile 1 images, as before R11H. A program
starts an image at any profile with `qd_start(name, profile)` (`QT_IMG_START`, 0x24). `profile = -1`
asks for the profile the image is linked for. Profile 0 is the shell's profile: a load for it is
refused unless the image is linked for it.

Profiles 2 and 3 exist only on the R9 map. On the default map they have size 0 and loads for them are
refused with `APP_ERR_NOPROFILE`.

## 3. Load rules (in order)

`aout_check_src` validates the file. Nothing in memory changes until validation passes.

1. File size at least the 32-byte header; OMAGIC (`0x107`) (else `APP_ERR_MAGIC` -21).
2. `trsize` and `drsize` zero (else `APP_ERR_RELOC` -22).
3. `entry` is the base of a profile (else `APP_ERR_ENTRY` -25). A base the memory map does not have
   gives `APP_ERR_NOPROFILE` -35.
4. Text, data and BSS are 16-byte aligned, and fit in the profile (else `APP_ERR_LAYOUT` -24).
5. The symbol table is a whole number of 12-byte records (else `APP_ERR_FORMAT` -23).
6. The file layout is exact: header, text, data, symbols, string table, with no truncation and no
   trailing bytes (else `APP_ERR_SIZE` -20).
7. A `_start` symbol equals the entry (else `APP_ERR_ENTRY` -25).
8. If a profile was requested and differs from the image's profile: `APP_ERR_NOTSLOT1` (-32) or
   `APP_ERR_NOTSLOT0` (-33) for profiles 0 and 1, otherwise `APP_ERR_NOPROFILE` (-35).

Then the load itself (`app_load_src`):

9. Arguments are validated (`APP_ERR_ARGS` -34).
10. The profile must have no live owner (`APP_ERR_SLOTBUSY` -28).
11. A job is created inactive. No job slot gives `APP_ERR_NOJOB` (-29); no heap for the stack gives
    `APP_ERR_NOMEM` (-30).
12. The profile range is reserved through the region allocator, owned by the new job. A conflicting
    reservation gives `APP_ERR_ADDROCC` (-36).
13. The image is copied. A failed copy releases the region, cancels the job and clears the owner.
14. The arguments and entry are installed, and the job is returned as a tagged id.

A failure at any step leaves no region, job or heap block behind.

Other codes: `FAT_ERR_NOFILE` (-44) when the name is not in the volume's root directory; `-2`
(`QD_ERR_NOJOB`) for an unknown or stale job id.

## 4. Job identity and exit

- A job id is `(tag << 16) | index`. The tag changes each time an index is reused.
- `QT_JOB_EXIT` (0x19) returns the exit status recorded under the full id, or a negative code.
  The exit record is kept under the full id, so a stale id gives `QD_ERR_NOJOB` only when its table
  index has been given to a newer job. Until then it still returns the recorded status.
- `QT_JOB_RECLAIMED` (0x1E) reports whether the job's table slot is free again.
- `QT_EXIT` (0x18) records the calling job's exit status.
- `QT_JOB_SLOT` (0x22) maps a table index to the tagged id of the job in it.
- `QT_TICKS` (0x20) returns the scheduler's tick count. The timer preempts running jobs on the tick.

`JOB_MAX` is 6: SYSTEM, IDLE, the shell, and room for two applications. With four profiles, a free
profile without a free job slot cannot occur in the R11H test set, so `APP_ERR_NOJOB` is not reached by
any guest test. It is reached in the code path only if a profile is free while the table is full.

## 5. Service traps used by R11H

| Trap | Value | Use |
|---|---|---|
| `QT_IMG_START` | 0x24 | `qd_start(name, profile)`; R11H addition |
| `QT_IMG_LOAD` | 0x1D | `RUN` by WIN1_ name (profile 1) |
| `QT_JOB_EXIT` | 0x19 | exit status of a job id |
| `QT_JOB_RECLAIMED` | 0x1E | job slot free check |
| `QT_EXIT` | 0x18 | record the caller's exit status |
| `QT_JOB_SLOT` | 0x22 | index to tagged id |

The SDK wrapper is `qd_start(const char *name, int profile)` in `sdk/include/qdos386.h`, implemented in
`sdk/lib/qdos.c`. It takes no arguments; the arguments-carrying form of `RUN` is unchanged.

## 6. Linking for a profile

`sdk/bin/qdos386-gcc -slot N` links for profile N (0 to 3) with `sdk/ld/slot<N>.ld`. Profile 2 links at
0x380000 and profile 3 at 0x500000, each with a 1 MiB size. A program linked for one profile is not
valid for another. The example programs in `sdk/examples/r11h/` show the forms:
`MULTIA.c` (slot 2), `MULTIB.c` (slot 3), `COLLIDE.c` (slot 2), `MULTI.c` and `NEG.c` (slot 1).

## 7. Shell START and PS: deferred

The plan's Phase 3 asks for shell `START` and `PS` commands. They are **not implemented in R11H**. The
launcher program (`MULTI`) uses `qd_start` directly, which is the mechanism the commands would have
used. Adding the commands would change the shell's command set and its help text, and the plan does not
require it to satisfy the concurrency criterion. A later round can add them on top of `QT_IMG_START`
without changing the loader.

## 8. Limits recorded here

- No relocation: an image runs at its link origin only.
- Profiles 2 and 3 are 1 MiB each (R9 map). The default map has none.
- The shell's `RUN` loads profile 1 only.
- Concurrency is bounded by the four profiles and the six-job table.
