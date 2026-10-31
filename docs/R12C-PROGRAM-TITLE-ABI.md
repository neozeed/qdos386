# QDOS/386 R12C — program title ABI

Kernels built with `-DQDOS_TITLES` (the R12C kernel, `make r12c-pc`) keep three names for every job, all
in kernel storage owned by the job's slot (`kernel/jobctl.c`, cleared whenever the slot is reused):

| name | set by | bound | used for |
|---|---|---|---|
| executable path | the loader: the file the launch actually opened after the PATH search and the `.AOU` suffix (for example `/GAMES/PHOON.AOU` for `PHOON`) | 71 bytes | `qd_exe_name`, task list column PROGRAM (as its base name `PHOON.AOU`) |
| compiled title | the program's `_prog_name` (see below) | 63 bytes | the title when no runtime title is set |
| runtime title | the program itself, `qd_set_title` | 63 bytes | the title |

**Shown title, precedence:** runtime title (if non-empty) > compiled title (if non-empty) > executable base
name. `qd_set_title(0)` or `qd_set_title("")` removes the runtime title, and the compiled title (or the name)
shows again. Kernel jobs show their own names: `SYSTEM`, `IDLE`; the boot shell shows `SHELL.AOU`.

**Bytes:** a title is 0..63 bytes of printable ASCII (0x20..0x7E). Any other byte is stored as `?` (no
encoding is interpreted; UTF-8 sequences become `??`). A longer string is cut after 63 bytes. The kernel
always NUL-terminates.

**Ownership:** a program changes only its own title; there is no call that sets another job's title.

## The C68 convention: `char _prog_name[] = "Hack-1.03";`

A program names itself as under C68 on QDOS:

```c
char _prog_name[] = "Hack-1.03";
```

Two mechanisms read it; both were tested with the historical toolchain (GCC 1.40, GAS 1.38, i386-ld 1.94),
which has no weak symbols (`docs/R12C-AUDIT.md`, linker probes in `logs/r12c/phase0/`):

1. **Symbol table, at load time (every binary, no relink).** Every QDOS/386 a.out keeps its symbol table (the
   loader requires `_start` in it). After a successful load the kernel scans the table forward for the
   global data symbol `__prog_name` (the C name with the a.out underscore; type `N_DATA|N_EXT` and a value
   inside the data section) and copies up to 63 bytes from the data section of the file
   (`aout_prog_name_src` in `kernel/aout.c`; at most 256 global data symbols are considered, names are read in
   string-table order so the file is never read backwards inside a pass). A binary linked before R12C that
   defines `_prog_name` therefore shows its title without being relinked (tested with `OLDT.AOU`, built with
   the R12B `crt0`: `tests/r12c/mk-oldt.sh`).
2. **crt0, at start (programs linked with the R12C SDK).** `sdk/lib/crt0_libc.s` calls
   `qd_prog_title(_prog_name)` before `main`. If the program does not define `_prog_name`, the archive member
   `sdk/lib/progname.c` (`char _prog_name[] = "";`, alone in its member) is linked instead and the call does
   nothing: an empty compiled title means "use the executable name". A program's own definition always
   wins: an archive member is only pulled when the symbol is still undefined.

Only the array form is supported: `char *_prog_name = "x";` would make the title the bytes of a pointer (shown
as `?` characters). A program without symbols (none exist in this toolchain) shows its executable name.

Programs linked before R12C without `_prog_name` (all frozen R12B binaries) run unchanged and show their
executable names.

## Calls (`sdk/include/qdos386.h`, `libqdos.a`)

```c
#define QD_TITLE_MAX 64
int  qd_set_title(const char *title);              /* bytes stored (0..63), or -14 bad pointer (title unchanged) */
int  qd_get_title(char *buf, unsigned int size);   /* length of the shown title; copies min(len, size-1) bytes + NUL */
int  qd_exe_name(char *buf, unsigned int size);    /* length of the executable path; same copy rule */
void qd_prog_title(const char *name);              /* crt0 only */
```

Trap `QT_JOB_TITLE` = 0x38 (`kernel/jobctl.h`): `a` = operation, `b` = string or buffer, `c` = buffer size.

| op | name | result |
|---|---|---|
| 0 | `JT_OP_SET` | runtime title; `b` = 0 clears it → bytes stored, or -14 when any byte up to the NUL (or the 63rd) is outside user memory (nothing changed) |
| 1 | `JT_OP_SET_PROG` | compiled title (crt0) → as op 0 |
| 2 | `JT_OP_GET` | shown title into `b`/`c` → its length; -14 bad buffer |
| 3 | `JT_OP_EXE` | executable path into `b`/`c` → its length; -14 bad buffer |
| other | | -15 |

Every pointer is checked with the same user-range test as the channel calls; the copy is byte by byte into a
kernel buffer, so a title can never point into kernel memory or change after the call. Older kernels answer -1
(unknown trap); `qd_prog_title` ignores the result, so an R12C program runs on an R12B kernel.

## Where titles are shown

* `QT_JOB_QUERY` version 2 (`qd_task_list`): 128-byte records with the title, its source and the program name
  (`docs/R12C-TASK-MANAGER-ABI.md`). Version 1 (`PS`) is unchanged.
* The shell's `TASKS /L`, and the task selector (Ctrl+Esc / `TASKS`).

## Tests

* `TITLET.AOU` (`sdk/examples/r12c/titlet/titlet.c`, 16 checks): compiled title, executable name, task list
  source, runtime set, empty and NULL clear, 100-byte cut at 63, control bytes as `?`, bad pointers (-14, title
  unchanged), truncated read, two copies with different runtime titles, 12 rapid exit/reuse rounds (a new job
  in a reused slot never shows the old title). `TITLET hold` changes its title every second (dynamic titles).
* `tests/r12c/run-host-progname.sh`: the symbol-table reader on R12C, R12B-crt0 and frozen R12B binaries.
* Gate 2: `tests/r12c/run-s2.sh` (QEMU), `docs/R12C-RUNTIME-REPORT.md`.
