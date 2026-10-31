# R13B Task Manager launcher (F2) and CLS

## F2 — Start Program

SYSTASK (the Task Manager, `kernel/taskmgr.c`) is still not a job: it runs in the SYSTEM job's loop and takes no job
slot. R13B adds **F2** while the selector is open:

* A line `Run: …` opens at the bottom (row 23) with a visible cursor. Keys: printable ASCII inserts (at most 70
  characters; further keys are ignored), Backspace, Del, Left, Right, Home, End edit; **Enter** starts; **Esc** closes
  the line and leaves the selector open ("start cancelled"); a second Esc closes the selector as before. While the
  line is open every key goes to it (Ctrl+Esc acts as Esc; Alt+F1..F4 still switch consoles and close the selector).
* The line is split like a shell command: a program name (in `"…"` if it contains blanks), then arguments at blanks,
  `"…"` keeping blanks. An unterminated quote or an empty line gives a message and starts nothing.
* **Lookup**: the kernel's one resolver (`docs/R13B-EXEC-RESOLUTION-ABI.md`) with the **boot shell's** current
  directory and `PATH` at that moment (the launch context: `img_launch_as`, `job_ctx_override`). The program must be
  linked for the canonical origin 0x240000, as for `START`.
* **Session**: a new private session (as `START … /NEW`), shown at once; the selector closes. When that session ends
  while shown, the display returns to the session that was shown when the selector was opened (if it still exists and
  was not replaced), else to console 0.
* **Environment**: a copy of the boot shell's environment; **directory**: the boot shell's.
* **Ownership and reaping**: the creator is SYSTEM (TASKS shows parent `-`). Like a job started with `RUN`/`START`, it
  gets no parent/child record, so nothing has to collect it: when it ends, the ordinary reclaim pass frees its address
  space, channels, heap, stack and session (`jobs.c reclaim_dead`), and its exit status stays readable by id until
  the slot is reused. It can be ended with F1 or `KILL`. It is not a child of the boot shell (the shell's `EXIT`
  warning and `WAIT` do not count it).
* **Errors** stay in the selector with the line kept for correction: `NAME: not found (here, in PATH or in /)`,
  `: not a valid a.out program`, `: not linked for the standard origin 0x240000`, `: no free job slot`, `: out of
  memory (no free page frames)`, `: not a directory …`, `: is a directory`, `: invalid name`, `: arguments refused …`.
* Stale ids: the list and every action still use tagged job ids (R12C); a job that ends while selected is reported,
  never confused with another.
* Counter: `QT_TASKMGR` op 3 (`TM_OP_LAUNCHES`) returns the number of programs started with F2.

The launch runs in the SYSTEM job with interrupts off (as a trap would), so the job table, the session table and the
loader are consistent; the selector's keys are queued meanwhile.

## CLS

Shell built-in (boot shell and SH): `CLS` (no arguments) sends the existing console control *clear whole session*
(`CHAN_CON_CLEAR`, 0) on the shell's own console channel. The kernel (`vcon_clear`) blanks **that session's** 80×25
cells with the session's current attribute and moves its cursor to row 0, column 0; the screen is loaded only if that
session is the one shown. Nothing else changes: not the font, not the attribute, not the session's text/rendered
mode (VGA text or R13A mode 12h — the presenter redraws a rendered session from its cells), not any other session.
No program writes video memory: other sessions' displays are untouched, and switching away and back shows the cleared
session from its cells. `CLEAR` is not added (nothing needed it).
