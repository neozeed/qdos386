# R12B system layout, program search, scripts and startup

R12B kernels (`-DQDOS_ENV`) and the R12B shell (`SHELL.AOU` built with `-DR12B_SHELL`, `build/apps/shell-b.aout`).
Everything runs at CPL 0; none of this is a security boundary.

## 1. System directory layout

`scripts/r12b-install.sh VOL.img` installs it on an existing FAT16 volume (host mtools; deterministic file times);
`scripts/r12b-sysvol.sh OUT.img` makes the example system volume (the R12B scratch volume + the layout, checked with
`fsck.fat -n` and the VFAT walker).

| Directory | Contents |
|---|---|
| `/BIN` | system programs: `EDIT.AOU`, `BASIC.AOU`, `LEAK.AOU`, `PINFO.AOU`, `ENVT.AOU`, `SHUTDOWN.AOU` |
| `/ETC` | `STARTUP.CMD` (from `fs/r12b-system/ETC/STARTUP.CMD`) |
| `/SDK/BIN` | small SDK sample programs (`HELLO.AOU`) |
| `/SDK/INCLUDE` | the SDK headers (`STDIO.H`, `QDOS386.H`, ...) |
| `/SDK/LIB` | `LIBC.A`, `LIBQDOS.A`, `LIBGCC-QD.A`, `README.TXT` |
| `/TMP`, `/HOME` | scratch and user files |
| `/GAMES` | `PHOON.AOU` |

`INCLUDE` and `LIB` are conventions for host-built programs only: **there is no compiler or linker on the guest**.
Programs that were in the root stay there (compatibility); `EDIT`, `BASIC`, `PINFO`, `ENVT` and `SHUTDOWN` are removed
from the root by the installer so that finding them proves the search. `LEAK.AOU` stays in the root as well, because
the R12 probe opens `WIN1:/LEAK.AOU` to count channel slots.

## 2. Program search (one resolver for every load)

`kernel/pathres.c` (pure C89, host-tested) is used by every load path of an R12B kernel: `RUN` (QT_IMG_LOAD), `START`
(QT_PG_START), QT_IMG_START, `qd_spawn` (QT_PROC_SPAWN) and therefore SuperBASIC `EXEC`/`EXEC_W` and every C program that
spawns. The PATH used is the **caller's** environment (kernel side, so it cannot be spoofed by the caller's memory
layout).

1. A name holding `/` or `:` (or a `WIN1_` / `WIN1:` channel name) is tried only as given; a relative path is relative
   to the caller's current directory.
2. A bare name is tried in the caller's current directory, then in each `PATH` entry in order (`;`-separated; blanks
   around an entry and empty entries are skipped), then in the root `/` (where R11-R12A programs live).
3. In each place the exact name is tried first; if its last component has no `.`, `name.AOU` is tried next. So `EDIT`,
   `edit`, `EDIT.AOU` and `/BIN/EDIT` all find `/BIN/EDIT.AOU`. Names are matched without regard to case (the FAT layer);
   the shell no longer folds the name, so `argv[0]` keeps the case typed.
4. The search goes on after "no such file" (-44), "not a directory" (-87), "is a directory" (-88) and a candidate the
   volume cannot hold (-46, -47, -90; a candidate over 128 bytes is skipped, never truncated).
5. Any other error ends the search and is returned (I/O error -40, bad cluster chain -43, no volume -18).
6. The first file found is loaded. **A malformed image is reported (e.g. -21), not skipped** — a later good copy is not
   used.
7. Nothing found: -44, or the name error of the name as given when that was the reason.

The R11M shell's own `/BIN` fallback is not compiled into the R12B shell (the kernel does it, with the documented order).
R12A and older kernels are unchanged (`win1_open_img` only).

## 3. Scripts: `CALL` / `SOURCE`

`CALL file` (or `SOURCE file`) runs the lines of a text file through the same dispatcher as typed lines (with `%NAME%`
expansion), in the shell process itself (so `SET` and `CD` in a script stay in effect afterwards).

| Rule | Value |
|---|---|
| file size | ≤ 4,096 bytes, read whole before the first line runs (a read error runs nothing) |
| line length | ≤ 159 characters |
| line ends | LF or CR LF; the last line may lack one; blank lines skipped |
| comments | `REM` (any case) alone or followed by a space |
| TAB | counts as a space |
| other control bytes | the script is refused before it runs |
| nesting | 4 levels (`CALL` inside a script) |
| `EXIT` in a script | ends that script only (status 0) |

**Errors that end a script** (and every script that called it): an unknown command, a line over 159 characters, an
expansion overflow, the nesting limit, a script too large or holding a control byte. The shell prints
`script <file> stopped at line <n>`; no later line runs. **Errors that do not**: an ordinary command failing (a missing
file for `TYPE`/`CD`/`RUN`, a bad variable name, ...) is reported and the script goes on, as typed commands do.

## 4. Startup: `/ETC/STARTUP.CMD`

The R12B shell runs it once, before its banner:

```
STARTUP: /ETC/STARTUP.CMD - press ESC within 2 seconds to skip it
...the script's output...
STARTUP: done (<n> commands)
QDOS/386 R12B shell. Type HELP.
```

* **Missing file**: `STARTUP: none (no /ETC/STARTUP.CMD)` and an ordinary shell (no wait).
* **Safe start**: ESC within about 2 seconds (200 ticks, polled every 5 ticks) skips the script:
  `STARTUP: skipped (ESC) - safe start, environment empty`. Other keys are discarded during the wait. This is the
  recovery from a startup that never returns (e.g. a program run in the foreground that waits forever).
* **Error**: the script stops at the failing line (§3); `STARTUP: stopped by an error - the shell is ready; ...`, then
  the banner and prompt. The lines before the error have taken effect.
* **Exactly once**: the shell is started once per boot by the kernel and runs the script before its first prompt. When
  the shell ends, the session ends (as in R12A); nothing restarts it, so there is no boot loop and no second copy of
  anything the script started.
* **BASIC-first** start: `START BASIC /CON:1` then `CON 1` in the script starts SuperBASIC on console 1 and shows it;
  console 0 keeps the shell (Alt+F1).

Default `STARTUP.CMD`:

```
SET PATH=/BIN;/SDK/BIN;/GAMES
SET HOME=/HOME
SET TEMP=/TMP
SET TMP=/TMP
SET INCLUDE=/SDK/INCLUDE
SET LIB=/SDK/LIB
REM START BASIC /CON:1
REM CON 1
ECHO PATH=%PATH%
```

## 5. Test harness options (QEMU capture)

`scripts/r12-qemu-capture.py` (defaults unchanged): `QEMU_BOOT_KEY=esc` sends the key once `QEMU_BOOT_KEY_AT` (default
`press ESC`) is on the screen; `QEMU_BOOT_WAIT` replaces `Type HELP` as the end-of-boot text (a BASIC-first startup
shows console 1).
