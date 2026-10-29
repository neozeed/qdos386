# R12 sessions and job ergonomics

Builds on `R11M-CONSOLE-ABI.md` and `R11M-JOB-CONTROL-ABI.md` (unchanged: four 80x25 virtual consoles, Alt+F1..F4 / Alt+1..4, tagged job ids, stale-id rejection, `START /CON:n`, `PS`, `WAIT`, `KILL`). R12 changes the shell commands, adds one opt-in kernel option, and states the ownership rules that were implicit.

Evidence classes: **guest** (QEMU full machine VGA/keyboard/IDE, `tests/r12/run-s3.sh`, `logs/r12/stage3/`), **independent** (mtools, `fsck.fat`, `tools/r11m-fatwalk.py --vfat`), **NOT TESTED**.

## 1. Ownership, focus, waiters, shell lifetime (the rules)

* **One shell, on console 0.** The shell is the slot-0 program loaded from the volume. Independent shells on other consoles are **not provided**: a second shell would have to run as a paged slot-1 job, and a paged job cannot start, wait for or kill jobs (R11M rule), so it would be a shell without job control. The supported model is one shell plus concurrent applications, each on its own console.
* **A job belongs to the console it was started on** (`START path /CON:n`; without `/CON` the shell's own console, which the shell then prints as "this console; /CON:n gives it its own"). Its CON channel is bound to that console when opened; children inherit it.
* **Input focus = the visible console.** Keyboard input goes only to the foreground console's 127-byte queue; a program on a background console that reads simply sleeps (state `CONWAIT` in `PS`). Alt+F1..F4 are consumed by the kernel and never reach a program. `CON n` does the same from the shell.
* **One waiter per console** (R11M): a second reader on the same console gets `-50` and backs off. Consequences: do not `START` an input-reading program on console 0 while the shell reads; give it a console. Killing a job that sleeps in the console wait releases the waiter at once: a new program can read that console immediately (guest: kill of a `CONWAIT` editor, relaunch on the same console).
* **Reclaim.** When the last user of the foreground console ends (exit or kill), the focus returns to console 0. Background consoles keep their text (`CONDUMP n` prints it).
* **Shell lifetime.** `EXIT` ends the shell, and the kernel then reports `SHELL: exited with status 0` and stops: other consoles no longer update (guest). Because that loses every started job and unsaved editor state, **R12 `EXIT` warns once when started jobs are still running** ("N started job(s) still running ... Type EXIT again to leave anyway") and leaves on an immediately repeated `EXIT`. Any other command in between re-arms the warning.
* **Kill ownership is unchanged**: any launched job may be killed from the shell; the shell itself and unlaunched system jobs cannot (`refused: not a started job`). Killing is immediate; the kernel's ordinary reclaim then releases channels, heap blocks, the address space and the console waiter. A killed editor does not save: its unsaved text is lost, the file on disk is untouched (guest + host byte comparison).
* **Active-console indicator: none.** No row of an application console is used for a status bar (an application owns all 25 rows). `CON` is the indicator: it prints a table with `*` on the visible console. Viewport unchanged.

## 2. Shell commands (changes)

| Command | R12 behaviour |
|---|---|
| `START path [args] [/CON:n]` | as R11M; the message now says how to look at the job: `job 131075 started on console 1 (Alt+F2 shows it)`. Errors: `no such console`, `no such file`, `no free image window (limit of started programs reached)`, `no free address space ...`, usage text. |
| `CON` | table: `this shell is on console 0, console N is visible`, then one line per console `* console 1  Alt+F2  EDIT.AOU [131075]` (started jobs and the shell; `(no job)` if none). |
| `CON n` | make console n visible; `usage: CON [0-3]` for anything else. |
| `PS` | unchanged columns (`ID SLOT STATE PRI CON AGE NAME`); started jobs that finished stay listed as `ENDED` with their status or `(killed)`. |
| `WAIT id` | as R11M, plus **Ctrl+C stops waiting** ("WAIT abandoned (Ctrl+C); job N keeps running"). Other keys typed meanwhile on the shell's console are dropped. Unknown/stale id: `no such job (unknown or stale id)`; not a started job: refused. |
| `KILL id` | as R11M (messages: `job N killed`, `no such job (unknown or stale id)`, `refused: not a started job`, `job has already ended`). |
| `KILL /CON:n` | **new**: end every started job on console n; `N job(s) killed on console n`, or `no started job on console n`. Needs no job id, so a console can be cleared with one command. |
| `EXIT` | warns once if started jobs are running (above). |

Job ids stay tagged (slot + reuse count): a recycled slot has a new id, so a stale id is rejected rather than hitting the new job.

## 3. Three paged applications at once: `QDOS_PG_WIN3`

R11M kernels have room for three paged jobs in the job table but the PC/QEMU frame pool (`0x380000..0x5FFFFF`) holds only **two** 1 MiB windows: the third `START` is refused with "no free image window". The demonstration (editor + ticker + Phoon, or EDIT + BASIC + COUNTER) needs three. R12 adds an **opt-in build option**: `-DQDOS_PG_WIN3` makes profile 3 two MiB (`0x500000..0x6FFFFF`), so the pool `0x380000..0x6FFFFF` holds three windows, still below the boot stack (top `0x7FFFF0`). `make r12-pc-pg` builds the PC kernels with it (`build/r12/pc-pg*/`). Default images and the R11M kernels do not define it and are byte-identical to FIX1 (`scripts/r11l-default-check.sh`: six hashes). The pool is reported by the boot line (`PAGING: on ... pool 00380000..00700000`). Needs at least 8 MiB of RAM (QEMU is run with 16, Tiny386 headless is started with at least 8).

Guest comparison (same sequence, same volume): R11M kernel: editor and counter start, the third `START` prints "no free image window (limit of started programs reached)"; R12 kernel: all three start and `PS` shows three paged jobs.

## 4. Evidence (guest, QEMU)

See `tests/r12/judge_s3.py` for the exact checks; results in `logs/r12/stage3/judge.txt` and `docs/R12-RUNTIME-REPORT.md`. Covered: the target demonstration (editor on 1, ticker on 2, Phoon on 3, shell on 0, switching with the editor's text and cursor intact), CON/PS output, WAIT cancel, kill of a job blocked in the console wait followed by a relaunch on that console, stale/unknown/bad ids and bad consoles, 22-line counters on two consoles finishing with no gap or repeat while focus switched 11 times, two editors on two consoles each receiving only its own typing (files compared on the host), four start/kill cycles with `LEAK.AOU` showing identical heap, channel slots and address spaces before and after, `EXIT` warning and session stop.

**NOT TESTED / not provided:** independent shells on other consoles; killing a job while the shell itself is blocked in `WAIT` (the shell cannot type; a killer job would have to be a non-paged job); more than three paged programs (refused, by design); behaviour on Tiny386 beyond the single release smoke.

## 5. Rule for paged programs that switch stacks (found in Stage 4)

A paged job may move its stack pointer only into **shared kernel-heap memory** (`qd_alloc`), never into its own window (BSS, `.data`, or stack inside the 0x240000 window). The scheduler loads the next job's page directory while it is still executing on the interrupted job's stack; a window-resident stack is not mapped in the other directories, so the next context switch triple-faults (seen under QEMU with `-d int,cpu_reset`: the first version of `BASIC.AOU` died when a second job ran). The job's own 64 KiB stack is already a heap block, which is why ordinary programs never meet this. Details and the fix are in `R12-SUPERBASIC-PORT.md` section 4.
