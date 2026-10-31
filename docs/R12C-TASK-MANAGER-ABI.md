# QDOS/386 R12C — global task selector (Task Manager) ABI

Kernels built with `-DQDOS_TASKMGR` (the R12C kernel; needs `QDOS_VCON`, `QDOS_JOBCTL`, `QDOS_TITLES`) have
**SYSTASK** (`kernel/taskmgr.c`), which shows every live job over the 80x25 text display and lets the user switch to
a job's console or end a job. SYSTASK is not a job of its own: `taskmgr_poll()` runs in the SYSTEM job (slot 0),
whose loop already wakes every tick to watch the shell, so it takes no job slot (programs keep the 13 slots they had
in R12A/R12B; a first version with a separate SYSTASK job made the R12A frame-exhaustion test run out of job slots
before frames, and was replaced). It is a text selector, not a window system: no application
buffer is drawn into, no job is paused, nothing is scheduled differently.

## Opening and closing

| how | effect |
|---|---|
| **Ctrl+Esc** (any console, any program, also while a program waits for input or BASIC is in `EXEC_W`) | opens it; a second Ctrl+Esc while it is open acts like Esc |
| shell **`TASKS`** | the same (trap `QT_TASKMGR` op 0); the command returns at once |
| `qd_taskmgr_open()` from a program | the same |
| **Esc** | closes it; the console that was in front is shown again from its own buffer, cursor where it was |
| **Enter** | shows the selected job's console (`vcon_switch`) and closes |
| **Alt+F1..F4** | closes it and shows that console (the R11M console switch keeps working) |

The decoder (`kernel/kbd.c`) reports Ctrl+Esc as its own event (result 3) to the keyboard IRQ
(`platform/pc/i8042.c`); nothing else about the key path changed. Tiny386's headless terminal sends only ASCII
and Esc-digit (Alt+digit), so there the selector is opened with `TASKS` and closed with Alt+1..4; its screen is
also written to the diagnostic stream as `TM|` lines (headless builds have no display).

**Screen ownership.** While it is open the console layer stops drawing (`vcon_overlay(1)`): programs keep
writing into their console buffers as before, but nothing reaches the display; SYSTASK composes its own 80x25
cell buffer and loads it with `vdisp_load`, cursor hidden. On close `vcon_overlay(0)` loads the foreground
console's buffer and cursor. No application data is overwritten, so closing restores the exact screen (tested
cell for cell, and the hardware cursor position, in Gate 1).

**Input ownership.** While it is open every key goes to SYSTASK's 32-byte queue (`taskmgr_key`, first thing
in `vcon_input`), including keys it does not use: no key typed in the selector reaches any program (Gate 1:
`zzz`, `qqq`, `xyz` typed in the selector never appeared in the shell, a C child's `fgets`, or EDIT).

## Screen

```
 QDOS/386 Task Manager                            11 jobs   ticks 8245
 ID       TITLE                        PROGRAM        STATE   CON  PARENT
 0        SYSTEM                       SYSTEM         SLEEP   0    -        K
 65537    SHELL.AOU                    SHELL.AOU      INPUT   0+   -        P
 ...
 65547    Disposable tick 36           TITLET.AOU     SLEEP   0+   65537
 Disposable tick 36: console 0, shared by 2 programs
 End Disposable tick 36?  Y = end it, any other key = keep it
 Up/Down select  Enter go to its console  F1 end job  Esc back  Alt+Fn console
```

* One row per live job, oldest first (kernel and boot jobs at the top; End selects the job started last), from
  a bounded snapshot of the job table taken with interrupts off (`snapshot()`, at most `JOB_MAX` = 16 rows; the
  list scrolls if it is longer than the 18 list rows). Columns: tagged job id, shown title (28 characters),
  program base name, state (`RUN`, `HELD`, `SLEEP`, `INPUT` = waiting in its console, `ENDED` = dead, not yet
  reclaimed), console (`+` = the console is shared by more than one program; kernel jobs are not counted),
  parent process id (`-` for none), `K` kernel job / `P` protected.
* Row 22: the selected job, its console, how many programs share it, whether it can be ended.
* Row 23: confirmation (white on red) or the result of the last action.
* The list refreshes every 25 ticks while open (titles and states change live) and after every key.

## Selection and actions

The selection is a **tagged job id**, not a row: it survives refreshes and reordering. On open it is the job
waiting for input on the console that was in front (else the newest program on that console).

| key | action |
|---|---|
| Up / Down / PgUp / PgDn / Home / End | move |
| Enter | `job_lookup(id)`; if the job still exists its console is shown, else "job N no longer exists: choose again" |
| F1 | protected job: "X is protected (SYSTEM, IDLE, the shell): it cannot be ended here"; otherwise asks "End X?  Y = end it, any other key = keep it" |
| Y (after F1) | `jobctl_kill(id)` (the same checks as `QT_JOB_KILL`/`KILL`): "job N ended; the other jobs go on", or "job N no longer exists: nothing was ended" (stale id: the job ended, or its slot was reused with a new tag), "had already ended", "is protected" |
| other key after F1 | "job N kept" |

If the selected job ends while the selector is open, the row disappears, nothing is selected and the message
says "job N has ended; Up/Down selects another job" — a following F1/Enter/Y can never act on another job.

**Protected:** jobs not created by a program launch (SYSTEM, IDLE, the boot shell) — the same rule
as `KILL`. Ending a job uses the normal kill path: its process, channels, memory and child records are
reclaimed by the kernel as for `KILL`; its parent sees the usual status.

## Calls

Trap `QT_TASKMGR` = 0x3A: op 0 open (0; -1 only before boot has finished), op 1 state (1 open / 0), op 2 the
number of opens. C: `int qd_taskmgr_open(void);`.

Task list for programs: `QT_JOB_QUERY` (0x2D) with version `c = 2` (`qd_task_list(buf, bytes)`): a 16-byte
header `{version 2, entry size 128, count, JOB_MAX}` then one 128-byte record per live job, `struct qd_task`:

| offset | size | field |
|---|---|---|
| 0 | 4 | tagged job id |
| 4 | 1 | slot |
| 5 | 1 | state (`JC_S_*`: 1 run, 2 held, 3 sleep, 4 dead, 5 waiting on its console) |
| 6 | 1 | priority increment |
| 7 | 1 | console (255: none) |
| 8 | 1 | flags: 0x01 launched, 0x02 paged, 0x04 the caller, 0x08 protected, 0x10 kernel job |
| 9 | 1 | programs (non-kernel jobs) on that console |
| 10 | 1 | title source: 2 runtime, 1 compiled, 0 executable name |
| 12 | 4 | parent process id (0: none) |
| 16 | 4 | dispatches |
| 20 | 4 | ticks since the job started |
| 24 | 32 | program base name, NUL-terminated |
| 56 | 64 | shown title, NUL-terminated |
| 120 | 8 | reserved (0) |

Version 1 (`PS`, 40-byte records) is unchanged. A buffer too small for all records gets as many as fit.

## Limits

* Text only, 80x25; no mouse; no graphics; no compositor.
* While open, the screen shows the selector only: a program's output appears when its console is shown again.
* The selector does not pause or resume jobs, and does not change priorities.
* SYSTASK runs in the SYSTEM job: while the selector is open SYSTEM handles keys every tick and redraws every
  25 ticks; it uses no job slot.
