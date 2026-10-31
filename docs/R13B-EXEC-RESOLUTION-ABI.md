# R13B executable resolution

One resolver decides which file a program name means: `kernel/pathres.c` `path_resolve` (R12B), called by the
kernel for **every** program load (`kernel/imgload.c`, `IMG_OPEN`). R13B adds no second resolver; it routes every
launch interface to it and fixes the shell side so that a bare name works everywhere.

| Interface | Kernel request | R13B |
|---|---|---|
| shell: a name as the command word (`PHOON`, `EDIT HELLO.C`, `"/APPS/My Tools/Arg Probe.aou" "x y"`) | `QT_IMG_LOAD` (as `RUN`) | new: an unknown command word runs a program in the foreground and waits for its status |
| shell `RUN name args` | `QT_IMG_LOAD` | unchanged |
| shell `START name args [/NEW|/CON:n]` | `QT_PG_START` | unchanged; `START SH /NEW` works because `SH.AOU` is linked at 0x240000 |
| `SH.AOU` (the dynamic shell) | same as the shell | the same source |
| SuperBASIC `EXEC`, `EXEC_W` | `QT_PROC_SPAWN` | unchanged |
| libqdos `qd_spawn` | `QT_PROC_SPAWN` | unchanged |
| Task Manager F2 (SYSTASK) | kernel load with the boot shell's directory and `PATH` | new (`docs/R13B-TASK-LAUNCHER.md`) |

## Order

1. **Built-in commands first.** The shell compares the command word (case folded) with its built-ins (`DIR`, `CD`,
   `RUN`, `START`, `CLS`, `DATE`, …) before anything else; a program called `DIR.AOU` is reached with `RUN DIR`.
2. **Quoted name.** A line that starts with `"` is a program name in quotes followed by arguments.
3. **Places.** For a name with no directory part (no `/`, no `:`, no `WIN1_`/`WIN1:` prefix): the caller's current
   directory, then each `PATH` element of the **caller's** environment in order (`;`-separated, blanks around an
   element ignored, empty elements skipped), then `/` (the R12B fallback, kept so that the root programs of older
   volumes keep working). A name with a directory part is opened only as given (relative names against the current
   directory): **an explicit path is never searched in PATH**.
4. **Candidates at each place.** If the last component of the name has **no dot**: the literal name, then
   name + `.AOU`. If it has a dot (an explicit extension, `TOOL.AOU`, `NOTE.TXT`): only the literal name — `.AOU` is
   **never** appended to an explicit extension.
5. The first candidate that opens is the program. A candidate that is missing, a directory, a path through a file, an
   invalid or too long name moves on to the next candidate/place; any other error (I/O, bad cluster chain) stops the
   search and is reported.
6. Matching is without regard to case (8.3 and VFAT long names, `kernel/fat16.c`). `argv[0]` is the name **as typed**
   (case and directory kept); the other arguments are passed unchanged — the shell splits at blanks, `"…"` keeps
   blanks and loses the quotes; at most 15 arguments and 432 bytes together with the name; a name of at most 64
   characters.

## Errors (shell)

| Case | Message |
|---|---|
| no candidate found, from the command word | `unknown command or program: NAME (not built in; not found here or in PATH)` (a script stops, as for an unknown command before) |
| no candidate found, from `RUN`/`START` | `no such file` |
| found, but not a valid a.out | `not a valid a.out image (code -2x)` |
| a part of the path is a file | `not a directory (a part of the path is a file)` |
| the name is a directory | `is a directory, not a program or file` |
| name over 64 characters | `invalid name`; a candidate over 127 bytes with its directory: `name too long (with its directory)` |
| job table full | `no free job slot` |
| no page frames / address space | `out of memory: no free page frames (code -60)` / `no free address space …` |
| arguments | `too many arguments (limit 15)`, `arguments too long (limit 432 bytes with the name)`, `unterminated quote` |

SuperBASIC reports a failed `EXEC`/`EXEC_W` with its own messages (`not found`, …) and `EXEC_STATUS`.

## Tests

Gate 2 (`tests/r13b/seq-g2-lookup.txt`, `judge_g2.py`) on a volume with `/ORDA/WHICH.AOU`, `/ORDB/WHICH.AOU`,
`/HOME/WHICH.AOU` (ARGV probes with different tags), `/ORDA/TOOL` (no extension) and `/ORDA/TOOL.AOU`,
`/ORDA/NOTE.TXT.AOU`, `/ORDA/DIR.AOU`, `/APPS/My Tools/Arg Probe.aou`: every row above and the order rules are
exercised through the command word, `RUN`, `START`, SuperBASIC `EXEC`/`EXEC_W`, `SH` and, in Gate 3, the Task
Manager.
