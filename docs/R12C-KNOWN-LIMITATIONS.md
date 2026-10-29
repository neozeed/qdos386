# QDOS/386 R12C — known limitations

Everything from `docs/R12B-KNOWN-LIMITATIONS.md` still applies (everything runs at CPL 0: nothing is security
isolation; 16 job slots; four text consoles; FAT16 without journaling). R12C adds:

## Task selector
* Text only (80x25), keyboard only; no graphics, no windows, no compositor (out of scope for R12C).
* While it is open the display shows only the selector; programs keep running and writing into their console
  buffers, which appear when a console is shown again.
* It cannot pause, resume or re-prioritise jobs; F1 ends a job with the same kill path as `KILL` (no graceful
  "please quit" message to the program).
* Protected (cannot be ended here): SYSTEM, IDLE and the boot shell — the same rule as `KILL`.
* SYSTASK runs inside the SYSTEM job (no extra job slot); while the selector is open it reacts to keys within one
  tick and redraws every 25 ticks.
* Tiny386's headless terminal cannot send Ctrl+Esc or arrow keys reliably: there it is opened with `TASKS` and
  closed with Alt+1..4; its screen goes to the diagnostic stream as `TM|` lines. `TASKS /L` is the scripted view.
* The list shows at most 16 jobs (the job table); the state shown is a snapshot refreshed every 25 ticks.

## Titles
* 63 printable ASCII bytes; other bytes are stored as `?` (no UTF-8, no code pages).
* The compiled title is the C68 array form `char _prog_name[] = "...";` only. Read from the symbol table at
  load time (every QDOS/386 a.out has one); at most 256 global data symbols are examined, so a program with more
  than 256 initialised global variables before `_prog_name` in link order shows its program name instead.
* A program can set only its own title. Titles are not inherited by children.

## SYNC
* See `docs/R12C-SYNC-DURABILITY.md` section 5: no crash-atomic FAT, no flushing of other processes' stdio
  buffers, no guarantee against power loss beyond the host's `fsync`/`fdatasync`.
* SYNC runs with interrupts off while the drive flushes: a slow host `fsync` (Tiny386) stops the guest's clock
  for that time; ticks are lost, not queued.
* The periodic sync is a system-wide kernel setting (also when set with `SET SYNC_INTERVAL` in one shell), is not
  kept across reboots, and is off by default.
* The Tiny386 runtime flush needs the R12C emulator patch (`patches/tiny386-r12c-flush.patch`); the R11M-FIX1
  emulator reports FLUSH CACHE as aborted and `SYNC` then fails (by design: no false success).
* QEMU reports a failed host flush as an aborted command (error register 4), the same code an unsupported
  command gives; the shell cannot tell the two apart.

## Tests
* QEMU is the primary test machine; Tiny386 runs are bounded (Stage 3 SYNC cases and one smoke).
* The R12 suites (Stages 1-4 and the 103-program SuperBASIC corpus) were not re-run on the R12C kernel (NA);
  the R12A and R12B suites were (`docs/R12C-RUNTIME-REPORT.md`).
* The negative no-SYNC kill cases are evidence of what is lost without SYNC, not gates.
