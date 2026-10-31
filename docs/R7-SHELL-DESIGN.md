# QDOS/386 R7 — shell design

The shell is a small command interpreter. It is an independent a.out program,
loaded into slot 0 by the kernel at boot, and it keeps running across commands.
It reads its input from the CON channel and runs its children from the WIN1_
volume in slot 1.

## 1. Boot

`shell_boot` (kernel/shellboot.c, built with `-DR7_SHELL_MODE`):

1. Banner `QDOS/386 R7`, descriptors, `input_init` (i8042 and IRQ1 on the PC),
   `sched_init`, heap `mem_init`.
2. `win1_mount`. Failure halts with 101.
3. `img_load_kernel("SHELL.AOU", 0)`: the same loader RUN uses, with slot 0
   required. Failure halts with 101 and prints the error code.
4. The IDLE job (lowest priority) and `cin_enable_poll`.
5. `timer_start`. From here the shell runs.
6. SYSTEM (the boot context) loops on `QT_JOB_EXIT` for the shell's job,
   sleeping one tick between checks (`QT_SUSPEND 1`). When the job ends it
   halts with the shell's exit status.

The shell is not a RUN child, so no RUN can replace it: RUN refuses slot-0
images, and the kernel loads the shell before any other job exists.

Halt codes from SYSTEM: the shell's own status (0 for EXIT, 3 for end of
input). 100 means the job ended without calling QT_EXIT (`JOB_ERR_NOSTATUS`).
101 means the shell could not be loaded or the volume was not mounted.

## 2. Commands

The line is read by `read_line`: characters are echoed, Backspace (0x08) and
Delete (0x7F) remove the last character, Enter (CR or LF) ends the line. A line
holds at most 79 printable characters; further characters are ignored, and
other control characters are dropped without echo. The line is folded to upper case before it is parsed,
so `type readme.txt` and `TYPE README.TXT` are the same command.

| command | effect |
|---------|--------|
| `HELP` | list the commands |
| `DIR` | list the root directory of WIN1_ (name and size of each file) |
| `TYPE <name>` | print a file, byte for byte, from its WIN1_ channel |
| `RUN <name>` | load `<name>` into slot 1, wait for it, print `exit status N` |
| `EXIT` | leave the shell with status 0 |

Any other word prints `unknown command: <word>`. A blank line prints nothing.

Error messages (exact text):

- `no such file`, `invalid name`, `bad cluster chain`, `volume not mounted` —
  from a channel open or read of a WIN1_ name (`say_name_error`).
- `too large: the limit is 8192 bytes` — RUN of a file over the cap. The
  kernel refuses it before reading (section 4).
- `not a slot 1 program (RUN loads slot 1 only)` — RUN of a slot-0 image.
- `not a valid a.out image (code N)` — the R5 loader rejected the bytes. N is the
  negative APP_ERR_* or FAT error.
- `slot 1 is busy`, `no free job slot`, `out of memory`.
- `program ended without an exit status` — the child ended without QT_EXIT.

## 3. Waiting without spinning

`con_getc` reads the CON channel. When the read returns `CHAN_ERR_AGAIN` (no
input yet), it calls `QT_CON_WAIT` with a flag byte, and the job sleeps until
input or end of input arrives. It then retries the read. The shell never loops
on an empty channel.

If `QT_CON_WAIT` is refused (negative), the shell sleeps one tick before it
retries. With one console reader the refusal does not occur in practice (see
the console ABI, section 5).

`wait_job` (for RUN) loops on `QT_JOB_EXIT` and sleeps one tick between checks
while the child is running. Once the status is known, it loops on
`QT_JOB_RECLAIMED` until the slot is free. The next RUN therefore never finds
slot 1 still in use.

## 4. RUN

`RUN <name>` calls `QT_IMG_LOAD` with the name. The kernel does the work and
returns either a job id or an error (console ABI, section 6). The shell maps
each error to a message and then returns to the prompt. A successful load
prints the child's output, then `exit status N`.

Children are linked for slot 1 (`apps/common/slot1.ld`), so they never overlap
the shell in slot 0. HELLO.AOU is the hello program linked for slot 1;
SLOT0.AOU is the same program linked for slot 0 and is used to check the
refusal.

The shell runs one child at a time. It waits for each child before printing
the next prompt, so a second RUN cannot start while slot 1 is busy.

## 5. End of input

Input ending (EOF) is reported as `end of input` and the shell exits with
status 3. The kernel then halts with status 3. A session that ends with EXIT
exits with status 0. The EOF path is documented in the console ABI, section 3.

## 6. Self-test build

`apps/shell/shell.c` compiled with `-DSHELL_SELFTEST` gives `SHELLT.AOU`. It
runs 19 checks of the command code against the volume and the loader, prints

    SHELL SELFTEST: 19 checks, 0 failed
    QDOS/386 R7 SHELL SELFTEST PASS

and exits with status 0 only when every check passed. The kernel's regression
suite (`tests/test_shell.c`) loads this image and checks its status. The
self-test uses no keystrokes, so it shows that the command code works but not
that a person can type into the shell.

The banner is `SHELL SELFTEST PASS`, not `INTERACTIVE PASS`. The name is
deliberate: the self-test is scripted in the guest, and no run in this release
involves a human at the keyboard.

## 7. Limits

- 8 KiB executable cap (IMG_MAX). Larger files are refused before they are read.
- Names: at most 10 characters through the `WIN1_` prefix (console ABI, section 9).
- One child at a time, slot 1 only.
- No pipes, redirection, arguments to programs, environment, or history.
- `TYPE` prints raw bytes; a binary file is printed as it is.
- Only the root directory is visible.
