# R12B SuperBASIC program execution: EXEC, EXEC_W and the job functions

`BASIC.AOU` (R12B build), host words of the extension table (R12B-SUPERBASIC-FILE-ABI.md §1), implemented with the
R12A process calls `qd_spawn` / `qd_wait` / `qd_poll` / `qd_kill`. Everything runs at CPL 0 (no isolation claim).

## 1. Statements and functions

| Form | Meaning | QL / extension |
|---|---|---|
| `EXEC name` | start the program and go on at once; the program's job id is remembered | QL name; QDOS/386: args and console below |
| `EXEC_W name` | start the program and wait until it ends | QL name |
| `EXEC name, args$` / `EXEC_W name, args$` | arguments: words separated by spaces; `"..."` or `'...'` keeps spaces (a BASIC string cannot hold `"`) | QDOS/386 extension |
| `EXEC name, args$, con` / `EXEC_W ..., con` | run the program on console `con` (0-3) | QDOS/386 extension |
| `EXEC_ID` | function: job id of the last EXEC / EXEC_W (0 before any) | QDOS/386 extension |
| `EXEC_STATUS` | function: exit status of the last EXEC_W, or of the last child collected by JOB_WAIT | QDOS/386 extension |
| `JOB_WAIT(id [, ticks])` | function: wait for a child of this interpreter: its exit status (≥ 0); `-1` still running after `ticks` (0 = poll, default: wait); `-2` not a child or already collected; `-66` it was killed | QDOS/386 extension |
| `JOB_KILL id` | end a child of this interpreter; anything else is error -2 "invalid Job" | QDOS/386 extension |
| `JOBS` | list the children this interpreter started that are still recorded (running, or ended and not yet collected) | QDOS/386 extension |

The name is found exactly like the shell's `RUN` (R12B-PATH-STARTUP-ABI.md §2: current directory, the interpreter's
`PATH`, the root; `.AOU` may be left out). The child inherits the interpreter's current directory and a **copy** of its
environment (R12B-ENV-PROCESS-ABI.md); no channel is inherited. Without `con` the child shares the interpreter's console
(output interleaves; a child that reads input should be given its own console).

## 2. Consoles and focus

* `EXEC` never changes which console is shown (no focus theft), whatever `con` is.
* `EXEC_W name, args$, con` with `con` ≠ the interpreter's console: if the interpreter's console was the one shown, that
  console is shown while the program runs and the interpreter's console is shown again when it ends. If the user had
  switched elsewhere, nothing is switched.
* While waiting, the interpreter does nothing else; Ctrl+C does not interrupt EXEC_W (end the child from the shell with
  `KILL`, then EXEC_W returns; EXEC_STATUS is then -66).

## 3. Status and errors

* EXEC_W succeeds whatever the program's exit status; the status is `EXEC_STATUS`. A program ended by the kernel (fault)
  or killed gives the kernel's status (-66 killed).
* Failure to start is a BASIC error at the statement (the program stops, as for any error):

| Cause (kernel code) | BASIC error |
|---|---|
| no such program (-44) | -7 not found |
| empty or over-long name, bad path | -12 bad name |
| not a valid program image (-20 ... -26, -31, -32, -61) | -15 bad parameter |
| console number outside 0-3, unterminated quote in args$ | -15 bad parameter |
| arguments over 432 bytes | -5 buffer full |
| out of page frames (-60) or kernel heap (-30) | -3 out of memory |
| job table full (-29), 8 children not yet collected (-69) | -2 invalid Job |

Nothing is left behind by a failed start (the R12A spawn rollback).

## 4. Limits

* 8 uncollected children per interpreter (kernel limit per parent), 32 system-wide; JOBS remembers 8 ids.
* Children of an interpreter that quits keep running (orphans) and are reclaimed by the kernel when they end.
* Several interpreters may each EXEC / EXEC_W at once; each sees only its own children (`JOB_WAIT`/`JOB_KILL` of another
  interpreter's child is -2).
