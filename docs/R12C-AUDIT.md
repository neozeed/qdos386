# R12C Phase 0 audit — jobs and consoles, program titles, storage write path

Base: frozen R12B `27a5d49d9ba6f8f9436d392feb668efbdaa7b552` (archive SHA-256 `af43204b…5d2f`, re-checked). Branch
`r12c-task-sync`. Baseline smoke (`logs/r12c/phase0/`), R12B kernel and binaries: QEMU — startup, EDIT on console 2
(type, save), BASIC on console 1 (`PRINT 6*7` = 42), Alt+F switching, `CON`/`PS`, a VFAT long-name `COPY` + `TYPE`,
`KILL /CON:1`, LEAK, clean `EXIT`; Tiny386 — the R12B smoke, guest shutdown, FAT copies identical, fsck and walker clean.

## 1. Jobs, ids, consoles, focus (what the task selector can do)

* **Job table**: `JOB_MAX` = 16 slots. Slot 0 SYSTEM (boot job, polls the shell), slot 1 the shell (`APP0`, a process
  since R12A but loaded by the kernel, so not "launched"), slot 2 IDLE; programs use the rest. Ids are tagged
  `(tag << 16) | slot`; `job_lookup` refuses a stale id, so an action keyed by id cannot hit a reused slot.
* **Enumeration**: `QT_JOB_QUERY` v1 (`kernel/jobctl.c`) copies 40-byte records (id, slot, state, priority, console,
  flags, dispatches, age, stack, a 15-character name: the launch basename or the kernel name). No titles, no parent.
  Parent ids exist in the R12A process records (`proc.c`, `QT_PROC_INFO`).
* **Kill**: `QT_JOB_KILL` ends a *launched* job (RUN/START/spawn): SYSTEM, IDLE and the shell are refused (-67), the
  caller itself (-68), an ended job (-69), a stale id (-2). It marks the job dead from another job's context
  (`job_mark_dead_other`), which must not be the job itself — a kill cannot be done from the interrupted context of the
  keyboard IRQ when the target is the job that was running. **So the selector runs as a kernel job, not in the IRQ.**
* **Consoles**: `VCON_MAX` = 4 text consoles (80×25 cell buffers, cursor, input queue). Every job belongs to one console
  (inherited, or chosen with `/CON:n` / `qd_con_child`); **any number of jobs may share a console**; only one job at a
  time may wait for input on a console (a second gets -50). The display shows exactly one foreground console;
  `vcon_switch` changes it (Alt+F1..F4 from the keyboard IRQ, `CON n` / `qd_con_focus` from programs). When the last
  job of the foreground console ends the focus returns to console 0.
* **Meaningful switching**: a job can only be "brought to the front" by showing its console. Jobs sharing a console
  cannot be told apart on the display (there are no per-job surfaces; those are R13). The selector therefore shows the
  console of every job and how many jobs share it.
* **Display path**: every console write updates its cell buffer and, for the foreground console only, VGA text memory
  (`vdisp_put` / `vdisp_load` / `vdisp_cursor`, four call sites in `vcon.c`). A selector that draws to VGA directly must
  first stop those writes (an *overlay* flag) and reload the foreground console when it closes; no console buffer is
  touched, so an application's screen is never overwritten.
* **Keyboard**: PS/2 set 1 through the i8042 IRQ1 handler → `kbd_feed` (Shift, Ctrl, Alt tracked; Alt+F1..F4 / Alt+1..4
  return the console hotkey code 2 and never reach a program; Esc is 0x1B, F1..F10 0x90..) → `cin_put` → `vcon_input`
  (to the foreground console). **Ctrl+Esc** is currently delivered as a plain Esc; it can be decoded in `kbd_feed` as a
  third hotkey class. QEMU `sendkey ctrl-esc` produces exactly that sequence. The Tiny386 headless frontend feeds ASCII
  from its input stream (Ctrl+Esc has no ASCII form), so the shell `TASKS` command is the way in there.
* **Kernel jobs**: `job_alloc(name, entry, priority)` creates one (IDLE is created that way after the shell); a kernel job
  sleeps with `QT_SUSPEND` (timed or indefinite) and is woken by setting its status (as `vcon_wake_job` does for console
  waiters). A selector job costs one job slot (16 → 15 for programs).

## 2. Program titles and the historical toolchain

* GCC 1.40 + GAS 1.38 + i386-ld 1.94 produce OMAGIC a.out. **GAS 1.38 has no `.weak`** (`Unknown pseudo-op: '.weak'`,
  `logs/r12c/phase0/linkprobe-w.s`); a.out has no weak symbols.
* `char _prog_name[] = "Hack-1.03";` compiles to a defined global data symbol `__prog_name` (type N_DATA|N_EXT).
* **Archive default**: a reference to `__prog_name` from the startup object is satisfied by the program's own definition
  when it has one; otherwise `ld` pulls a one-symbol archive member that defines `char _prog_name[] = ""`. Probed with the
  real tools (`linkprobe-*`): `Hack-1.03` program → its string; plain program → the empty default; `char
  _prog_name[64];` (a common symbol) → zero-filled BSS, i.e. empty → no title. The default member must contain nothing
  else (a member that is pulled for another symbol would clash: ld 1.94 then reports "multiple definition" but still
  exits 0 — a silent hazard the design avoids).
* Existing binaries need nothing: no symbol is required of a program, and a binary linked before R12C simply never sets
  a title (the kernel shows the executable name).
* `_stack` (C68) is **not** interpreted (R12C does not change stack sizing).

## 3. Storage write path (what SYNC can reach)

| Layer | State held | Controlled by the OS? |
|---|---|---|
| application stdio (`FILE` buffers in libc) | unwritten bytes in the process | only by that process (`fflush`); the kernel cannot reach another process's buffers |
| channel / FAT16 (`kernel/fat16.c`, `win1.c`) | **write-through**: every data, FAT (both copies) and directory sector is written to the device when the write call runs, **except** the directory entry (size, first cluster) of a file open for writing, which is written at flush/close/job exit (`fat16_flush`, `f->dirty`) | yes |
| block driver (`platform/pc/disk.c`) | ATA PIO, one sector per command, waits for DRQ/ready; no cache | yes |
| ATA device cache | QEMU: writes go to the host file via the block layer; Tiny386: `fwrite` into a host `FILE*` buffer | `FLUSH CACHE` (0xE7) |
| QEMU (8.2.2) | `-drive file=…,format=raw` = cache mode writeback: host page cache | QEMU implements 0xE7 → `bdrv_flush` → host `fdatasync` |
| Tiny386 (R11M-FIX1) | stdio buffer of the image `FILE*` (lost on SIGKILL) | **0xE7 is not implemented: aborted (ERR, ABRT)**; `ide_flush_all` (fflush + ferror + fsync, every writable image) runs only at the end of the emulator (0x8900 shutdown, SIGTERM/SIGINT) |
| host disk | device cache | not controllable from the guest |

So a kernel SYNC can: (1) write the pending directory entries of all files open for writing, (2) then issue ATA FLUSH
CACHE and check its status. To reach the Tiny386 host file without exiting, the emulator needs a flush command in its
IDE model (the natural place: 0xE7, calling `fflush`/`fsync` on that drive's image, reporting failure as ERR) —
a small extension of existing device semantics; the 0x8900 shutdown path stays as it is. Concurrency: every file system
operation runs inside one trap with interrupts off, so a SYNC trap is serialised with all of them and sees every earlier
write completed.

## 4. Periodic work

The timer ISR runs the scheduler and poll list with interrupts off; disk I/O there would block everything. A kernel job
(like the selector job) can sleep with a timed `QT_SUSPEND` and call the same SYNC trap as a program — a safe context
with its own stack. One kernel job can serve both the selector and the periodic sync.

## 5. Decisions

1. **SYSTASK**: the task selector (overlay over the console renderer, own key queue) and the optional
   periodic sync. Opened by Ctrl+Esc (IRQ decodes, wakes SYSTASK) or `TASKS` (trap). *(Implementation note, Stage 5:
   first built as its own kernel job; that cost programs a job slot - the R12A frame-exhaustion regression then hit the
   job limit before the frame limit - so SYSTASK now runs in the SYSTEM job's loop, which already wakes every tick.)*
2. Titles: per-slot kernel strings (executable basename, compiled title, runtime title; 64 bytes each), a new trap, a v2
   job query; `_prog_name` through the archive-default member and one call in `crt0_libc.s`.
3. SYNC: `QT_SYNC` (flush directory entries, ATA FLUSH CACHE, status); Tiny386 gets 0xE7/0xEA → per-image flush, with an
   environment-variable fault injection for tests.
4. Everything new is under `QDOS_TASKMGR`, `QDOS_TITLES`, `QDOS_SYNC` (`make r12c-pc`); R12B/R12A/R12 kernels and the
   default images stay byte-identical.
