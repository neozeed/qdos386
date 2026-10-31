# R13B audit (Stage 0)

Base: frozen R13A `951352c73d20f4be2f593c974fbb47e6061154aa` (archive `6c966941…`, re-checked). Branch
`r13b-shell-usability`. Read from the R13A source; the baseline QEMU smoke is `logs/r13b/stage0/` (R13A VGA kernel
`f802435e…`, R13A volume, `tests/r13b/seq-p0-baseline.txt`).

## 1. Why `START SHELL /NEW` fails

* The boot shell `SHELL.AOU` is linked with `apps/common/slot0.ld`: origin **0x200000** (historical slot 0, 224 KiB
  image limit). Every other program (`slot1.ld`, the SDK crt0) is linked at **0x240000**, the canonical origin.
* The kernel's process loader (`kernel/proc.c`, `proc_load`) can build a private address space at either origin: on
  `QDOS_DYNPROC_SHELL` kernels the old slot-0 range is pool memory (`shell_is_process = 1`), and `want_slot = -1`
  accepts any historical origin. That path is used only by `QT_PROC_SPAWN` (`img_spawn_request`: libqdos
  `qd_spawn`, SuperBASIC `EXEC`/`EXEC_W`) and by the kernel's own boot load of the shell (`want_slot = 0`).
* The shell's own launch paths require slot 1: `RUN` uses `QT_IMG_LOAD` → `img_load_request` → `want_slot = 1`
  (refused `APP_ERR_NOTSLOT1` −32: "not a slot 1 program"); `START` uses the R11J veneer `QT_PG_START` →
  `img_pg_start_request` → `proc_load(…, 1, …)`, which maps −32 to `PG_ERR_ORIGIN` **−61**, printed by the shell as
  "not linked for the standard origin: cannot be started in the background".
* Baseline smoke: `START SHELL /NEW` → the −61 message; `START SHELL.AOU /NEW` and `RUN SHELL` → "not a slot 1
  program (RUN loads slot 1 only)". So the restriction is the *link origin of the shell image* together with the
  slot-1 requirement of RUN/START, not a missing loader capability.
* Consequence for R13B: the same shell source linked with `slot1.ld` (0x240000) is an ordinary program for RUN, START
  and spawn. The boot shell keeps its 0x200000 link and its boot path (`shellboot.c` → `img_load_kernel(…, 0)`).

## 2. Boot-shell assumptions in the shell and kernel

| Place | Assumption | Effect on a dynamic shell |
|---|---|---|
| `shellboot.c` | SYSTEM waits for the **boot shell's** job id; when it ends the machine halts with its status | only the boot shell; a dynamic shell ending is an ordinary process exit |
| `shell_env.inc` `startup()` | runs `/ETC/STARTUP.CMD` (2 s ESC window) at every shell start | must not run in a dynamic shell (it inherits the environment; re-running SET FONT=, START … would repeat system setup) |
| shell `main()` | opens `CON` = the caller's session; banner | fine: `/NEW` gives it a private session |
| `jt[]` job table, `exit_blocked()` | per shell instance (static data in its own image) | private per process: independent |
| cwd | kernel per job (`win1_cwd_inherit` in `job_alloc`) | inherited from the creator, then independent |
| environment | kernel per process (`env_inherit` in `proc_load`) | snapshot of the creator's, then independent |
| `slot0.ld` 224 KiB limit | image size | the shell is 35 KiB |
| no fixed addresses | the shell uses only its own static data and traps | none found (`grep` for 0x200000/0x240000 in `apps/shell`: none) |

Children started by `RUN`/`START` get **no** parent record (R11M semantics kept by R12A: only `QT_PROC_SPAWN` makes
records). Every dead job is reclaimed by the ordinary reclaim pass (`jobs.c reclaim_dead`: address space, channels,
heap, stack) whoever its parent is; its exit status stays readable by id (`QT_JOB_EXIT`) until the slot is reused.
So a shell that ends or is killed leaves its started children running as orphans that need no reaping; `QT_PROC_SPAWN`
children's records are dropped with the parent (`proc.c`, "orphans").

Focus: when the shown private session ends, `vcon.c sess_free` switches to console 0 (the boot shell), even if
the session was started from another session.

## 3. Launch paths and the resolver (R12B)

`kernel/pathres.c` `path_resolve` is already shared by every kernel load (`IMG_OPEN` in `imgload.c`: RUN
`QT_IMG_LOAD`, START `QT_PG_START`, `QT_IMG_START`, `QT_PROC_SPAWN` = libqdos `qd_spawn` and SuperBASIC
`EXEC`/`EXEC_W`). Rules as implemented: the name as given relative to the caller's directory; if that fails and the
name has no directory part, each `PATH` element (caller's `PATH`, `;`-separated, blanks trimmed), then `/`. For a
final component without a dot, each place tries the literal name, then name + `.AOU`. An explicit directory
(`/`, `:` or a `WIN1_`/`WIN1:` prefix) is not searched in PATH. Names are matched without regard to case
(VFAT/8.3 lookup in `fat16.c`). Missing: the shell does not treat an unknown command word as a program ("unknown
command: PHOON"; `RUN PHOON` and `RUN HELLO` work). Quoted names: `RUN "Long Name.aou"` only.

## 4. Task Manager (R12C/R13)

`kernel/taskmgr.c`: SYSTASK is not a job — `taskmgr_poll()` runs in the SYSTEM job's loop; Ctrl+Esc (keyboard IRQ)
or `QT_TASKMGR` opens it, keys go to its private queue, it draws in VGA text mode over the display and restores the
foreground session on close. Keys: arrows/Home/End/PgUp/PgDn, Enter (switch), Esc, F1 (end task, Y to confirm).
Actions use tagged job ids. No launching exists. A launch from SYSTASK would run with SYSTEM as the current job:
SYSTEM has no environment (no `PATH`) and directory `/`.

## 5. Console clear

`CHAN_CON_CLEAR` (channel control 0x11, libqdos `qd_con_clear(ch, QD_CLR_ALL)`) → `vcon_clear(con, VCLR_ALL)`:
blanks the **caller's own session buffer** with its current attribute, cursor to (0, 0), and loads the display only
if that session is shown. In R13A rendered mode the presenter redraws from the cells, so the same call serves both
text back ends. No shell command uses it ("unknown command: CLS" in the baseline).

## 6. Clock

* `kernel/clock.c` (R11F): anchor + exact tick accumulation, states NONE / RUNNING / PINNED / OVERFLOW; trap
  `QT_WALL_TIME` 0x23; libc `time()` → `qd_wall_time()`; `0xFFFFFFFF` = unavailable.
* `kernel/clock_boot.c`: the anchor comes **only** from the build macro `QDOS_CLOCK_EPOCH`; R13A kernels do not
  define it, so the clock is NONE. No CMOS RTC code exists (`proc.c` reads CMOS 0x30/0x31 for the RAM size only).
* Phoon's message comes from `build/phoon-r11f/src/phoon.c` line 540 (the R11F portability patch of
  `third_party/phoon/orig/phoon.c`): `time(0) == (time_t) -1` → "%s: no clock is configured; give a date", exit 1.
  Baseline: `RUN PHOON` prints exactly that. Its date syntax is `date_parse` (e.g. `"11 Jan 2024 11:57:00"`);
  ISO `2026-10-29` is refused ("illegal date/time") — R13B does not change Phoon.
* QEMU: MC146818 RTC on ports 0x70/0x71, BCD by default (register B bit 2 = 0), 24-hour (bit 1 = 1), century in CMOS
  0x32; `-rtc base=…` sets a known start. Tiny386 (`misc.c cmos_update_time`): host UTC in BCD, century in 0x32,
  register B 0x02 (24-hour, BCD), refreshed by its update cycle.
* R11F boundary behaviour (2100 not leap, 2101, last second 2106-02-07 06:28:14 = 0xFFFFFFFE) lives in libc and the
  host clock tests (`tests/host/r11f`); pinned test kernels use `QDOS_CLOCK_EPOCH`.
* Oracle: the R11F host reference — the same patched Phoon source built with host gcc `-DOS_SYSV` and a fixed
  `time()` shim (`docs/R11F-RUNTIME-REPORT.md` §4.3).

## 7. Baseline smoke (`logs/r13b/stage0`)

`LEAK heap=726304`; `START SHELL /NEW` → −61 message; `START SHELL.AOU /NEW`, `RUN SHELL` → not a slot 1
program; `PHOON` → unknown command; `RUN PHOON` → no clock; `CLS`, `DATE`, `TIME`, `HELLO` → unknown command;
`RUN HELLO` → exit status 7; `TASKS /L`: SYSTEM, SHELL.AOU, IDLE.

## 8. Plan

1. `SH.AOU` = the shell source with `-DR13B_SHELL -DR13B_DYN`, linked at 0x240000, in `/BIN`; no STARTUP; the boot
   shell is rebuilt from the same source (`shell-f`, `-DR13B_SHELL`, 0x200000) with the new commands; R13A's
   shell-e and all frozen images stay byte-identical.
2. Unknown command word → the same launch path as `RUN` (kernel resolver); `"quoted path" args`; focus returns to
   the launching session when a private session ends.
3. SYSTASK F2: an edit line, launched by the kernel with the **boot shell's** environment and directory as the launch
   context, owner SYSTEM (no record needed: reclaimed at exit), in a new private session. `CLS` via
   `CHAN_CON_CLEAR`.
4. CMOS RTC read at boot (stable double read, BCD/binary, 12/24 h, century) → clock anchor, unless a build epoch is
   given; `DATE`/`TIME` print `time()`; unavailable stays unavailable.
