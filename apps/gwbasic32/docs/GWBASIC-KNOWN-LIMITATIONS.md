# GW-BASIC on QDOS/386 — Known Limitations

Status as of the Gate 0–4 pass on the unmodified KCTX-FIX2 kernel. Every entry was checked on the
Win32 build (`make test`), the QDOS build (`tests/qdos/run_qdos.py --suite all`), or the statement
probe (`tests/coverage/`). Where a statement says "observed", it describes what this build prints.
**The original Microsoft GW-BASIC was not available in this environment, so no output in this
project was compared with it, except where stated.** A behaviour that is not in the test suite is
not claimed.

Status key: **fixed** = verified and tested; **open** = a known gap, not fixed in this pass;
**native** = a deliberate difference from DOS GW-BASIC; **stub** = accepted or refused with a
defined message, without the feature.

## 1. Kernel finding (not a port defect)

- **save_write_error fails by design (open, kernel).** A single injected block-write fault
  (QEMU blkdebug `write_aio` errno 5, once) during `SAVE` reaches the BASIC program as
  `Disk write protected` (kernel errno 30, EROFS), not `Device I/O Error`. The minimal C program
  `fopen("IOE.DAT","w")` under the same injected fault also returns errno 30, while the
  `WRTTEST ioerr` create path reports EIO. The port was not changed to hide this. Evidence and the
  hand-off are in `docs/GWBASIC-KCTX-FIX2-HANDBACK.md` section 4 and `evidence/save_write_error/`.

## 2. File handling (SAVE, LOAD, MERGE)

- **Native image format (native).** SAVE writes the program as held in memory (4-byte absolute
  link fields and a 4-byte end link). DOS tokenized `.BAS` files are neither read nor written.
- **ASCII form (fixed).** `SAVE "name",A` and `MERGE "name"` work (`save_ascii`, `save_ascii_big`,
  `merge_ascii`, `merge_errors`). A direct statement inside a MERGE file stops the merge with
  `Direct statement in file`; lines before it are kept (`merge_errors`).
- **Names are 8.3, flat root only (QDOS, open).** The QDOS legacy `WIN1_` names accept 8.3 names in
  the volume root. A 12-character name fails with `Device I/O Error`. Subdirectory paths are not
  mapped (`SAVE "NODIR/X.BAS"` → `Device I/O Error`). The DOS message for these cases was not
  verified here.
- **Empty name (fixed).** `SAVE ""` and `LOAD ""` → `Bad file name`.
- **Missing file (fixed).** `LOAD "NOSUCH"`, `MERGE "NOSUCH"` → `File not found`.
- **Malformed file (fixed).** A 1-byte file, a file with no line structure, and a file whose
  link runs past its end all report `Bad file mode` and leave the program unchanged (`load_errors`,
  `load_errors_83`). The QDOS 8.3 variant uses `BADSTR.BAS` because a 9-character base name cannot
  exist on the flat volume.
- **LF-only ASCII file (open, observed).** A MERGE file whose lines end in LF without CR is read as
  one overlong line and reports `Line buffer overflow`. The file used to probe this was the original
  listing as published (LF line ends); the port was not changed. Whether DOS GW-BASIC reads LF-only
  files is not verified.
- **Overwrite and truncation (fixed).** `SAVE` replaces an existing file and truncates it to the
  new size (`save_load`).
- **Disk full, directory full (fixed).** `save_diskfull_empty`, `save_diskfull_partial`,
  `save_dirfull` pass. A failed SAVE leaves no partial file.
- **Read-only media (open, not tested).** The `save_readonly` case is reported as **SKIP**: QEMU 8.2.2
  cannot attach a read-only IDE hard disk in this setup. The `-80` mapping in `platform_qdos.S`
  (`plat_wrerr`) and `sav_error` is therefore not exercised.
- **Persistence (fixed).** A saved program is identical (SHA256) after a fresh boot of the volume.
- **Sequential and random file I/O (stub).** `OPEN`, `CLOSE`, `PRINT #`, `INPUT #`, `WRITE #`,
  `FIELD`, `GET`/`PUT` with a file, `LOC`, `LOF`, `EOF`, `KILL`, `NAME`, `LSET`, `RSET`, `CHAIN`,
  `BSAVE`, `BLOAD` → `Advanced Feature`. `FILES` lists the current root directory.
- **Printer (stub).** `LPRINT`, `LLIST`, `LPOS` → `Advanced Feature`. `LLIST` once looped forever
  on its own error message; that is fixed (`llist_unsupported`).

## 3. Directory and environment statements (keyword gap)

The statements below are not tokenized in this build and give `Syntax error`:
`CALL`, `CHAIN`, `CHDIR`, `MKDIR`, `RMDIR`, `ENVIRON`, `ERDEV`, `VIEW`, `WINDOW`, `PMAP`, `PALETTE`,
`SHELL`, `GET`, `PUT`. Their token numbers collide with `STMDSP` slots 149–158, so they cannot be
added without renumbering the dispatch table, which was not done in this pass (open).
`SYSTEM` is implemented and exits to the QDOS shell (`system_exit`).

## 4. Tokenizer: keyword directly followed by letters or digits (open)

Keywords immediately followed by a letter or digit are not tokenized in this build. Observed:
`10 FOR I=1TO3` and `20 IF 1>0ANDI<2 …` → `Syntax error in 10`. Writing `FOR I=1 TO 3` works.
The original Microsoft listing of Super Star Trek uses the unspaced form, which suggests the original
interpreter accepts it, but that was **not verified** here. The historical Star Trek run uses a
spaced copy of the listing (see `docs/GWBASIC-RUNTIME-REPORT.md`, section 5). Every other change
to that listing is recorded in `historical/PROVENANCE.md`.

## 5. Numerics

- **DP transcendental functions return single precision (open).** The double-precision path of
  the transcendental functions (`SIN`, `COS`, `TAN`, `ATN`, `EXP`, `LOG`) returns a single-precision
  result: `PRINT SIN(1#)` gives ` .841471 `. `PRINT EXP(1)` and `PRINT EXP(1#)` both give
  `2.718267` (observed), which is wrong in the fifth significant digit (e = 2.7182818…); the accuracy
  of `EXP` was not checked against the original. Integer,
  single and double arithmetic, `CINT`, `CSNG`, `CDBL`, `FIX`, `INT` edge cases and literals
  (`1E3#`, `1E30#` → `1D+30`) are tested (`int_edge`, `int_ops`, `int_single`, `dp_ops`,
  `dp_var_print`, `arith_mixed`).
- **RND** sequence was not compared with the original interpreter.

## 6. Time, date and device status (stubs returning zero or fixed values)

- `TIME$` → `00:00:00`, `DATE$` → `00-00-2048`, `TIMER` → `0`. There is no clock binding in
  Stage 4 (`DEVGDAT`, `DEVGTIM`, `DEVSDAT`, `DEVSTIM` in `src/stage3stubs.S`).
- `ERR` and `ERL` read as `0` in direct mode (observed). They were not tested inside an error handler.
- `STICK(0)` returns a meaningless value (observed `-13724`). `STRIG`, `PEN` → `Illegal function call`.
- `INKEY$` returns the empty string when no key is waiting (observed).

## 7. Graphics and sound (stubs)

- Observed: `PSET`, `PRESET`, `WIDTH`, `MOTOR`, `EDIT` → `Advanced Feature`. `LINE`, `COLOR 7`
  and `SCREEN 0` → `Illegal function call`. Only the text screen mode exists.
- Stubs in `src/stage3stubs.S` (not probed): `CIRCLE`, `PAINT`, `DRAW`, `POINT`, `PSET`,
  `PRESET`, `LSET`, `RSET`, `FIELD`. Each jumps to `ADVERR`.
- `SOUND` and `BEEP` are accepted and silent (no sound output).
- `KEY LIST` prints the function-key labels (observed).

## 8. Port I/O and machine access (stubs)

- `INP`, `OUT`, `WAIT`, `USR`, `VARPTR` → `Illegal function call` (refused).
- `POKE` is accepted and has no effect; `PEEK(1)` returns `0`. There is no access to machine memory.

## 9. Other statements and functions

- `ERASE` and `DELETE` of a non-existent item → `Illegal function call` (observed).
- `RENUM` → `Illegal function call` (not implemented).
- `GET`/`PUT` (graphics and file): bare `GET` is accepted and `GET 1` is a `Syntax error` (see
  section 3; the two forms disagree).
- `CVS`, `CVD`, `CVI`, `MKI$`, `MKS$`, `MKD$` → `Advanced Feature` (not implemented; so `MKI$`
  etc. cannot be used with `LSET`/`FIELD`).
- `OPTION BASE`, `RANDOMIZE`, `TRON`, `TROFF`, `SWAP`, `DEFSTR`, `DEFINT`, `DEFSNG`, `DEFDBL`,
  `WHILE`/`WEND`, `RESUME`, `ON GOTO`, `GOSUB`, `RETURN`, `RESTORE`, `READ`, `DATA`, `STOP`, `CONT`,
  `CLEAR`, `FRE`, `POS`, `SPACE$`, `STRING$`, `INSTR`, `HEX$`, `OCT$` are implemented and have
  test cases or probes (`docs/GWBASIC-LANGUAGE-COVERAGE.md`).

## 10. Historical programs

- Super Star Trek runs on Win32 and QDOS, but only the spaced copy (section 4). The original listing
  has no clear redistribution licence, so the package does not contain it (URL and SHA256 only).
- The fractal listing (MIT, Eric Furst) runs unchanged apart from CRLF line ends and a trailing
  CTRL-Z, which the source tree stores as the files were published. The line-end change is recorded in
  `historical/PROVENANCE.md`.
