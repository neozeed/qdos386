# R12B known limitations

## 1. Not security isolation

Every process, the shell and SuperBASIC run at CPL 0 (as in R12A). The environment is kept by the kernel and copied
with pointer checks, and a program cannot change another program's environment through the traps — but any program can
write kernel memory. No part of R12B is a security boundary.

## 2. Environment

* 32 variables, 2,048 bytes of entries, names of 1-31 `[A-Za-z0-9_]` characters, values of 0-255 bytes, per process.
  No wildcard listing, no export flags: every variable is inherited.
* Names are case-insensitive and stored upper case (`SET path=x` lists as `PATH=x`).
* `SET NAME=` sets an empty value; unlike MS-DOS it does not remove the variable (`UNSET` does).
* `getenv` returns one static 256-byte buffer, overwritten by the next call (C89 allows this).
* `setenv`/`unsetenv` are extensions and do not set `errno`; `system()` still returns -1 (no command processor call).
* The environment is not saved anywhere: each boot starts empty until `/ETC/STARTUP.CMD` runs.
* The diagnostic failure injection (`QT_ENV_LIST` index 0xFFFFFFFE) is in the shipped kernel; it only makes the next
  copies fail, and only when a program asks for it.

## 3. Shell expansion and scripts

* One expansion pass; no escape inside double quotes (`"%PATH%"` is never expanded); no `%1`-style script arguments;
  no `IF`/`GOTO`/labels in scripts; no `ERRORLEVEL`.
* Trailing spaces of a typed line are dropped; to keep them in a value, quote it.
* Scripts: 4 KiB, 159-character lines, 4 levels of `CALL`.
* A script runs inside the shell: a program it `RUN`s in the foreground that never ends blocks the shell (at boot, ESC
  skips the startup script).
* The startup bypass is a 2-second ESC window; a headless boot without a keyboard simply runs the script.

## 4. Program search

* `PATH` entries and names are limited by the volume's 64-character path limit; candidates over 128 bytes are skipped.
* Only the `.AOU` suffix is tried automatically. No `PATHEXT`.
* The root is always searched last, even when `PATH` does not name it (compatibility with R11-R12A volumes).
* The R12B shell does not run a program by typing its bare name: `RUN` / `START` are still needed.

## 5. SuperBASIC

* `LOAD` and `MERGE` work only on typed (direct) lines, not inside a running program (-19).
* `LOAD`/`MERGE` accept only numbered lines (the QL would execute unnumbered lines as direct commands; that cannot be
  staged). No `LRUN`, `MRUN`, `SEXEC`, `LBYTES`, `SBYTES`, `OPEN`/`CLOSE` of files, no device names (`flp1_`, `mdv1_`).
* `SAVE` writes LF line ends only; it is not atomic across a reset between its two renames (`~SBxxxx.BAK` then holds the
  old program).
* `EXEC_W` cannot be interrupted from the BASIC console (Ctrl+C); end the child from the shell with `KILL`.
* `EXEC` children share the interpreter's console unless a console number is given; their output interleaves.
* `JOBS` remembers 8 children; the kernel allows 8 uncollected children per interpreter (`JOB_WAIT` collects).
* Arguments for `EXEC`: a BASIC string cannot contain `"`, so `'...'` groups words with spaces.
* The SB4 core's own limitations are unchanged (apps/superbasic/docs/KNOWN-LIMITATIONS.md), e.g. READ needs DATA as the
  first statement of its line and RESTORE to position the cursor.

## 6. System volume

* The installer is a host script (mtools); there is no guest installer.
* `/SDK/INCLUDE` and `/SDK/LIB` hold the cross SDK's files for reference; there is no compiler or linker on the guest.
* `LEAK.AOU` is kept in the root as well as `/BIN` because the R12 probe opens `WIN1:/LEAK.AOU`.

## 7. Not tested

* A FAT close failure during SAVE, a read error during LOAD or script reading, power loss between SAVE's renames.
* An I/O error during the program search (host-tested only: `tests/host/r12b/test_pathres.c`).
* Tiny386: one bounded smoke only (as required); everything else is QEMU.
* Real hardware.
