# QDOS/386 R8 - application ABI

Status: implemented on branch `r8`. Source: `apps/common/app_start.s`
(copied to `sdk/lib/crt0.s`), `apps/common/appcalls.h`, `sdk/include/qdos386.h`,
`kernel/aout.c` (`args_write`, `args_check`), `kernel/jobs.c` (`QT_APP_ARGS`).

## 1. Entry

The loader creates a job whose entry point is the `_start` symbol of the image
(checked against the a.out entry field). `_start` is in the crt0 object, which
is linked first.

`_start`:

1. `QT_APP_ARGS` (0x1F) with a = address of a local word.  The kernel returns
   argc in EAX and stores the argv array address in that word.
2. Pushes argv, then argc, and calls `main(argc, argv)` (C convention of GCC 1.40:
   arguments on the stack, return value in EAX).
3. Passes the return value of `main` to `QT_EXIT` (0x18) in EBX.
4. Returns.  The kernel ends the job; the exit status is readable with
   `QT_JOB_EXIT` (0x19) once the job has ended.

The job's stack is the kernel's per-job stack (16 KiB, `APP_STACK_BYTES`).  The
program must not touch BSS itself; the loader zeroes it before the job runs.

## 2. `main`

    int main(int argc, char **argv)

- `argc` counts argv[0].  With no arguments, `argc` is 1.
- `argv[0]` is the program name as the kernel received it.  The shell folds the
  name to upper case, so `RUN args.aou` gives `argv[0] = "ARGS.AOU"`.
- `argv[1]` .. `argv[argc-1]` are the RUN arguments in order.
- `argv[argc]` is NULL.
- Every string is NUL-terminated and lives in the argument block of the slot.
  It is valid until the job exits.  After exit the slot is released, and the
  block is overwritten when the next program is loaded into that slot.  Copy
  anything that must outlive the job.
- Images linked for R5/R7 may keep `int main(void)`; the extra arguments are
  ignored.  The R5/R7 compatibility path is therefore unchanged.

Return value: the exit status.  The shell prints `exit status N`.

## 3. Argument block

Each slot holds its own block at its top, 512 bytes (`APP_ARG_BYTES`):

| Offset in block | Contents |
|---|---|
| +0 | argc |
| +4 | address of the argv array |
| +8 | argv array: argc pointers, then NULL (16 entries at most) |
| +80 | argument strings, NUL-terminated, at most 432 bytes (`APP_ARG_STR_OFF`, `APP_ARG_STR_MAX`) |

The block is written after the image is copied, so an image cannot be
partly written when the arguments are refused.  Addresses are absolute, inside
the slot.

## 4. Limits

| What | Limit |
|---|---|
| argc | 16, including argv[0] |
| user arguments | 15 |
| total argument text | 432 bytes, including the name and every NUL |
| RUN line | 80 characters (shell) |
| quoting | double quotes only; no escapes |

Refused with `APP_ERR_ARGS` (-34): an argument block that is not NUL-terminated,
over 432 bytes, or with more than 16 strings.  The shell reports these earlier
with its own messages: `too many arguments (limit 15)`, `arguments too long`,
`unterminated quote`.

## 5. RUN argument syntax

    RUN <name> [argument ...]

- Words are separated by spaces (one or more).
- A word that starts with `"` runs to the next `"`.  Spaces inside are kept.
  The closing quote must be followed by a space or the end of the line.
- There is no escape character: a quote cannot appear inside a quoted word.
- An unterminated quote is refused: `unterminated quote`.
- Only the command word and the RUN name are folded to upper case.  Arguments
  keep their case: `RUN ARGS.AOU Mixed` gives `argv[1] = "Mixed"`.
- Empty quoted words (`""`) are passed as empty strings.

This is deliberately modest.  It is not a POSIX shell: no globbing, no
variables, no redirection, no pipes, no `$`.

## 6. Environment

There is no environment.  `argv` is the only way to pass data to a program.

## 7. Traps available to applications

| Trap | Number | Use |
|---|---|---|
| QT_SUSPEND | 0x08 | sleep for ticks; -1 = until reactivated |
| QT_JOB_EXIT | 0x19 | exit status of a job (JOB_ERR_RUNNING while it runs) |
| QT_CH_OPEN | 0x14 | "CON" or "WIN1_" + 8.3 name |
| QT_CH_CLOSE / WRITE / PUTC | 0x15 / 0x16 / 0x17 | channel output |
| QT_CH_READ / SEEK | 0x1A / 0x1B | file channels (read at most 256 bytes per call) |
| QT_CON_WAIT | 0x1C | wait for console input |
| QT_MEM_ALLOC / FREE / FREE_BYTES / VALIDATE | 0x10-0x13 | QDOS heap (not malloc semantics) |
| QT_EXIT | 0x18 | exit status (crt0 does this for you) |
| QT_APP_ARGS | 0x1F | argc and the argv array address (crt0 does this for you) |
| QT_TICKS | 0x20 | timer ticks since boot, low 32 bits |

Programs should use `qd_*` from `sdk/include/qdos386.h`, not raw traps.

## 8. What R8 does not provide

- No `stdio`, `printf`, `FILE`, `malloc`, `realloc`, `calloc` or `getenv`.
- No `exec` from an application; only the shell starts programs.
- No writes to WIN1_ (read only).
- No per-application heap: `QT_MEM_ALLOC` uses the kernel heap, which is shared.
- No preemption guarantee visible to the application beyond the timer; a
  CPU-bound program is preempted by the scheduler, which is verified by the R3/R5
  tests, not by the R8 program itself.
