# R14A — QL C compatibility map for Hack 1.03

R14A extends the R14 QL source-compatibility layer (`docs/R14-QL-COMPAT-ABI.md`) with the C68 runtime interfaces
Hack's QL port uses. As in R14 this is **source** compatibility: the QL Hack source is compiled for i386 and linked
with QDOS/386 libraries; no 68000 code, QDOS traps or QL memory map exist. Nothing returns a pretended success.

## 1. Build

`third_party/hack103-ql/r14a/Makefile.qdos386` (opt-in; `Makefile.qdos` unchanged), driven by
`scripts/r14a-build-hack.sh`, which copies the pristine tree, applies `hack-qdos386.patch` **if it exists — it does
not: 0 source lines changed**, builds and writes `symbols.txt` (`scripts/r14a-symreport.py`), `hack.map`,
`source-changes.diff` (empty) and the executable's SHA-256.

| `Makefile.qdos` | `Makefile.qdos386` | Why |
| --- | --- | --- |
| `gcc` (QDOS m68k GCC + C68 libc 4.24.5), `-O2` | historical i386 GCC 1.40, `-O` (its only level) | host toolchain of QDOS/386 |
| `QDOS` predefined by the compiler | `-DQDOS` | GCC 1.40 does not define it |
| `-DMSDOS` | `-DMSDOS` | unchanged |
| C68 include path | `sdk/qlcompat/include`, `sdk/include`, `port/include` | QL headers first, then the QDOS/386 libc and POSIX headers |
| `copy /Y onames.z onames.h` | `tr -d '\r' < onames.z > onames.h` | GCC 1.40's preprocessor rejects CR |
| `x68k.o` | not linked | its `signal()` duplicates the libc's; `kill()` comes from the shim |
| `-lgcc -lc -lvt -lgcc` | `c68vt.o`, `libc68.a libm.a libc.a libgcc-qd.a libqdos.a` | §2 |
| `ld -o hack` (QDOS executable) | `i386-ld -N -T slot1.ld -e _start -M` → `HACK.AOU` + `hack.map` | a.out at the canonical origin 0x240000, own address space (R12A) |

Image: text 205,008, data 10,304, bss 15,440 bytes (SHA-256 `18c4ab6c…`, `logs/r14a/gate1/HACK.AOU.sha256`); symbol table kept (the kernel reads `_prog_name` from it: the
Task Manager shows **Hack-1.03**, R12C title rule). The build is deterministic (two builds byte-identical, same map).
Compiler warnings (11, `logs/r14a/gate1/build.log`) were reviewed: K&R signal-handler and pager pointer types
(`signal()` result unused), `#ifdef MSDOS && notdef QDOS` (GCC and C68 both read `#ifdef MSDOS`), `*cmd == 0377`
always false (plain `char` is signed in both compilers — checked: GCC 1.40 i386 loads `char` with `movsbl`), a
pointer/integer conditional in `steal.c`; none changes behaviour against the QL build.

## 2. Dependency matrix

Status: **B** build verified, **H** host tested, **G** guest executed (QEMU), **D** differentially verified against
the QL binary under QLBox. Exact external list: `logs/r14a/gate1/symbols.txt` (59 externals, 0 unresolved).

| Original symbol (QL provider) | QDOS/386 provider | Adapter | Status |
| --- | --- | --- | --- |
| `__ANSICONF__` (LibVT / C68 `ansicondef.h`) | `c68vt.c`: `ANSICONDEF __ANSICONF__ = {VT100, 0}` | C68 4.24.5 type and bit values | B G |
| `ANSI_conwrite` (LibVT, not supplied) | `c68vt.c` | VT100 subset → console calls (`docs/R14A-LIBVT-ANSI.md`) | B H G D |
| `_conwrite` (C68 `fcntl/_conwrite.c`) | `libc68.a` (`stdio.c -DQDOS_C68_CONWRITE`) | C68 signature and >0/<0/0 protocol; default NULL | B H G |
| end message `_endmsg` (C68 `defaults/endmsg.c`, `unistd/_exit.c`: "Press a key to exit" when the program owns its console window) | `libc68.a`: `char *_endmsg`, an `atexit` hook armed on first console use | owns = private session with one user (`qd_surface_info`); in the shell's session nothing is shown, as for C68's inherited channel; NULL suppresses | B G D |
| `_prog_name` (C68 start-up) | kernel title rule (R12C) | none | B G (Task Manager "Hack-1.03") |
| `_cmdparams`, `_cmdchannels`, `_cmdwildcard`, `_stackchannels` (C68 start-up vectors) | defined by Hack, never referenced | none (data only) | B |
| `_condetails` (C68 console window) | defined by Hack, not used | QDOS/386 session = whole 80×25 screen | B; documented difference |
| `getch` (C68 `lattice/getch.c`) | `c68vt.c` | `qd_con_getkey` (kernel wait); ENTER 13 → 10 | B G |
| `getlogin` (C68) | `c68vt.c` → `"root"` | the QL build greets "Hello root" with no USER (measured) | B G D |
| `kill`, `signal` (`x68k.c` stubs) | `c68vt.c` `kill` → −1; libc `signal` → `SIG_ERR` | no delivery, as on the QL | B |
| `getchar`, `fflush`, `printf`, `puts`, `putchar`, `fputs`, `fputc`, `sprintf`, `setbuf`, `fopen`, `fgets`, `fscanf`, `fprintf`, `fclose`, `rewind` | `libc68.a` / `libc.a` | console reads wait in the kernel (CR → LF) | B G D |
| `open`, `creat`, `write`, `lseek`, `close`, `unlink`, `fstat` | `libc68.a posixio_c68.o` (= R11L `posixio.c`) | relative names in the current directory; `O_BINARY` = 0 | B G |
| `read` on a file (C68: `io_fstrg`, the whole count unless the file ends) | `libc68.a posixio_c68.o` (`-DQDOS_C68_POSIX`) | repeat the ≤ 256-byte channel read until the count, the end or an error. **Found in Gate 3**: without it Hack's `mread()` saw "Read 256 instead of 3520 bytes." while saving, and the save was unusable | B G |
| `unlink` of a file the program still has open (Unix/C68: allowed; Hack's `mread()` deletes the bad save it is reading) | `libc68.a posixio_c68.o` (`-DQDOS_C68_POSIX`) | QDOS/386 answers EBUSY: the name is kept (at most 4) and deleted when a descriptor is closed and at exit. **Found in Gate 4**: a truncated save stayed and every start said "Error restoring old game." | B G |
| `getpid` (1), `getuid` (0) | `libc.a` | save file `save/0root` as on the QL (uid 0) | B G D |
| `getenv` | `libc.a` (R13B environment) | USER/LOGNAME unset → `getlogin` | B G |
| `time`, `localtime` | `libc.a` (R13B CMOS clock) | seed and moon phase | B G |
| `srand`, `rand` | `libc.a` (C89 LCG) | C68's generator differs: same game flow, different dungeons | B G |
| `malloc`, `realloc`, `free`, `qsort`, `atoi`, `abs`, `exit`, string functions, `errno`, `perror` | `libc.a` | none | B G |
| `<qdos.h>` `struct WINDOWDEF` | `sdk/qlcompat/include/qdos.h` | struct tag added to the R14 typedef | B |
| `<qptr.h>` | empty header | Hack uses nothing from it | B |
| `<ansicondef.h>` | `sdk/qlcompat/include/ansicondef.h` | C68 4.24.5 declarations + `struct UFB` leading fields | B |
| `ESRCH` | `sdk/include/errno.h` (3) | only named by dead code (`MULTIUSER_LOCK` undefined) | B |

## 3. Data layout, endianness, files

* **Save, level and bones files** are raw `write()`s of structures (`struct monst`, `struct obj`, `struct rm levl[][]`,
  `struct you`, `struct flag`, pointers fixed up by `restmonchn`/`restobjchn`). Between the m68k and i386 builds the
  byte order, `int`/pointer sizes (both 32-bit), struct padding and **bitfield allocation order** (`Bitfield(x,n)
  unsigned x:n`: MSB-first on m68k GCC, LSB-first on i386) all differ, so **QL save/bones/level files are not
  readable by the QDOS/386 build and vice versa**. Hack's own checks (`uptodate`, `hackpid`, level number) are kept
  and not extended; no converter is provided. Native save → quit → relaunch → restore is tested (Gate 3).
* `uptodate()` compares the save's time with the game binary's (`gethdate` returns early under `MSDOS`, so `hbuf` is
  zero and every save is "up to date", as on the QL).
* Names: `save/0root`, `root.1`…, `bones_NN`, `record`, `perm`, `data`, `help`, `hh`, `rumors`, relative to the
  current directory (`CHDIR` undefined). QDOS/386 resolves them in the job's cwd on WIN1 (VFAT long names, case
  preserved; `save/` is a real subdirectory). Hack must be started in its playground (`CD /GAMES/HACK` then `HACK`, or
  `START HACK /NEW` from there: the cwd is inherited).
* Text: the data files are read with `fgets`; LF files (the source tree's) are used. No character translation.
