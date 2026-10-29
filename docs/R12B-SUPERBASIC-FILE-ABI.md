# R12B SuperBASIC program files: LOAD, SAVE, MERGE, DIR

`BASIC.AOU` (R12B build: `-D QB_R12B`, `apps/superbasic/qdos/qb_ext.c`) on the SB4 core.

## 1. How the words are added (the core change)

In QDOS, `LOAD`, `SAVE`, `MERGE`, `EXEC`... are entries of the SuperBASIC name table, and a host adds more with
`BP.INIT`. The SB4 core has no such table: these words lexed as plain identifiers and a statement made of an unknown
name gave -12 ("bad name"). R12B adds the hosted counterpart, nothing else:

* `SbExtName` (`sb_state.h`): `{ name, proc, func }`, a table ended by a NULL name, installed by the host in
  `SbState.ext` (NULL by default).
* `stmt_ident` (`sb_interp.c`): for a name that is **not** a DEFine PROCedure/FuNction of the program (those always win,
  with their existing -7/-12 rules) and not an array being assigned, a table procedure is called with the lexer after the
  name; it parses its arguments up to the statement separator.
* `parse_primary` (`sb_expr.c`): a name that is not a DEFine FuNction, variable, array or string array may be a table
  function, with or without `(...)`.
* `sb_new()` (what NEW does), `sb_in_direct_mode()` and `sb_ext_find()` are exported for the host.

The full difference against the supplied SB4 sources is `apps/superbasic/qdos386-core-changes.patch` (4 files);
`apps/superbasic/verify-core.sh` reports the rest identical. With no table installed the core behaves exactly as before:
the SB4 corpus (103 programs, 3 examples, the documented-discrepancy files and the unit checks) gives a **byte-identical
report** on the host with the original and the changed core (`tests/host/r12b/sb-core-corpus.sh`), and the guest corpus
runner `BASTEST.AOU` (which installs no table) is re-run in QEMU (R12B-RUNTIME-REPORT.md).

No BASIC semantics were invented: the statements and the order of name resolution are unchanged; only names that
were errors before can now be host words.

## 2. Names

`LOAD name`, `SAVE name`, `MERGE name`, `DIR [name]`: `name` is a string expression (`SAVE "/HOME/My First
Program.bas"`), or a bare name that is not a variable, taken literally (`LOAD prog_bas` opens the file `prog_bas`; no
QL-style `_` to `.` translation and no default extension). 1-64 characters (the volume's path limit). Relative names are
relative to the interpreter's current directory (inherited from the shell that started it). Long VFAT names with
spaces work.

## 3. SAVE

* Text: exactly what `LIST` prints — `<number> <canonical text>` and **LF** after each line, in line order. An empty
  program gives an empty file. (LOAD accepts LF and CR LF.)
* Procedure: write `~SBxxxx.TMP` beside the target (xxxx = the interpreter's job id, so two interpreters do not
  collide), flush and close it, **read it back and compare** with the program text, then rename the old file to
  `~SBxxxx.BAK`, rename the new file to the target, remove the backup.
* On any failure before the last rename the target is untouched and the temporary file is removed (tested: disk full on
  a nearly full volume, `drive full`, the old file reloads unchanged). A failed final rename puts the old file back.
* **Not atomic**: a reset between the two renames leaves the old program as `~SBxxxx.BAK` and no target.
* Allowed in a program as well as typed.

## 4. LOAD and MERGE

* **Direct mode only** (typed lines). In a running program they give -19 "not implemented" (a QDOS/386 restriction:
  replacing the program under the running line is not supported).
* The file (≤ 64 KiB) is read whole, then every line is **staged** into a scratch interpreter state with the same store
  function as a typed numbered line (`sb_prog_store`: canonical form; a number alone deletes that line). Only if every
  line is accepted is the program changed: LOAD does NEW and takes the staged program; MERGE moves the staged lines in,
  a line number already present being replaced (as typing it would). Variables are kept by MERGE.
* Refused (program unchanged, message `LOAD: line <n> of the file refused; the program is unchanged`):
  a line without a number (-21 bad line — **QDOS/386 restriction**: the QL executes unnumbered lines of a loaded file as
  direct commands, which cannot be staged), a number over 32767 (-21), a control byte other than TAB (-21), a line over
  255 characters (-5 buffer full), a file over 64 KiB (-5).
* Blank lines are ignored; CR LF and LF line ends; the last line may lack its newline.

## 5. DIR

`DIR [name]` lists a directory (default: the current one) with long names, sizes and `<DIR>` marks, then
`<n> file(s), <m> dir(s)`. Output goes to the console.

## 6. Errors

| QDOS/386 code | BASIC error |
|---|---|
| -44 no such file | -7 not found |
| -46 -47 -87 -88 -90 -91 bad/too long name, not a directory, is a directory | -12 bad name |
| -81 disk full, -82 directory full | -11 drive full |
| -80 write-protected | -20 read only |
| -83 exists | -8 already exists |
| -84 busy | -9 in use |
| -85 write error, -40 read error, -43 chain, -45 size, -18 no volume | -16 bad or changed medium |
| a non-string name | -15 bad parameter |

The core prints the message as for any other error (`not found`, `drive full at line 20`, ...).

## 7. Not tested

A failing close (no fault injection in the FAT layer), a read error during LOAD, power loss between the renames.
