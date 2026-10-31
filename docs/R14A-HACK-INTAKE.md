# R14A intake — the QDOS/C68 Hack 1.03 port

Branch `r14a-hack-ql-port` from the frozen R14 (source `43a1f448…`, packaging `2279de87…`). This file is Gate 0:
the exact build inputs, what the QL port depends on, the patch proposal and the licence assessment.

## 1. The supplied archive

| Item | Value |
| --- | --- |
| File | `hack.zip` (user-supplied with the R14A handoff) |
| Size / SHA-256 | 682,504 bytes / `aa1e48e1ba13a32fdf5f6e4e79ee05d988d4de64dea62d2184dd795a250b7233` |
| Entries | 128 (124 files, 4 directories), one top directory `hack/` |
| Listed before extraction | `unzip -l`: no absolute paths, no `..`, no links |
| Pristine copy | `third_party/hack103-ql/orig/` with `orig-SHA256SUMS` (all 124 files); 5 withheld files (§4) are listed there and in `orig-WITHHELD.txt` and are not in the repository |

Contents (by inspection, nothing inferred from file names alone):

* **Hack 1.0.3 source** (`version.c`: "1.0.3 - last edit"; `date.h`: "Tue Jul 23 1985"; `hack.6` dated 31 March 1985):
  63 `.c` files, headers, `onames.z` (the generated object-name header, CR LF), `makedefs.c`.
* **Build files**: `Makefile.qdos` (the QL build), `Makefile`, `Makefile.NT`, `.emx`, `.msc`, `.wat`, `.x68`, OpenWatcom
  project files (`hack.tgt`, `hack.wpj`, `hack.mk*`, `hack.lk1`), `linkfile*`, `db.sh`, `hack.sh`.
* **Runtime text** (LF, matching this source: "Hack & Quest data file - version 1.0.3", "Welcome to HACK! (description
  of version 1.0.3)"): `data`, `help`, `hh`, `rumors`.
* **Prebuilt binaries — never executed on the host**: `hack` (224,376 bytes, the QL 68000 executable built by
  `Makefile.qdos`, `GAME = hack`; strings: `Hack-1.03`, `QDOS!`, `C68 libc.a v4.24.5 (gcc)`), `makedefs.exe` (PE32
  Windows console program) and `makedefs.obj`. The QL `hack` is run only inside the QLBox emulator as the oracle (§6).
* **`test/`** — a QL run directory: `hack` (224,372 bytes; an earlier QL build: 32,212 bytes differ, 4 bytes shorter),
  `nethack` (a QDOS executable of NuPC NetHack), NetHack 1.0 text files `data`, `help`, `hh`, `rumors` (CR LF; "NetHack
  & Quest data file - version 1.0", "Welcome to NetHack!"), empty `perm`, and `bones/`, `save/` with empty `keep.me`.
* Licences: `LICENSE` (BSD-3-Clause, neozeed 2022), `copyright` (Stichting Centrum voor Wiskunde en Informatica 1985,
  BSD-style), `copyright-jf` (Jay Fenlason 1982, BSD-style).

Not in the archive: C68/LibVT headers or libraries, a README for the QL port, `news`, `record`. The C68 runtime the QL
binary was linked with — **C68 libc 4.24.5** — was supplied earlier (R11, `libc-4.24.5.tar.bz2`); its
`include/ansicondef.h`, `include/fcntl.h`, `fcntl/_conwrite.c`, `lattice/getch.c` and `defaults/libvt.s` are the
reference for the C68 interfaces below. **LibVT** (`-lvt`, the code behind `ANSI_conwrite`) was not supplied: only
the libc's dummy `libvt.s` (an empty `LIBVT_VER` label) exists.

## 2. The QL build (`Makefile.qdos`)

```
CC=gcc   CFLAGS = -DMSDOS -O2
OBJ = LIB1..LIB5  (decl apply bones hack cmd do do_name do_wear dog eat end engrave fight invent pcioctl
                   lev makemon mhitu mklev mkmaze mkobj mkshop mon monst o_init objnam options
                   pager potion pri read rip rumors save search shk shknam steal
                   termqd timeout topl track trap unix u_init vault wield
                   wizard worm worn zap version rnd alloc x68k pctty)
ld -o hack main.o $(OBJ) -lgcc -lc -lvt -lgcc
onames.h: copy /Y onames.z onames.h
```

* Compiler: a QDOS GCC (m68k) with the C68 libc 4.24.5 ("(gcc)" build); version not stated. C dialect: K&R with a few
  `//` comments; `QDOS` is predefined by the QL compiler (only `-DMSDOS` is on the command line).
* Not linked although present: `termcap.c`, `termpc.c`, `tty.c`, `ioctl.c`, `msdos.c`, `makedefs.c`.
* No assembler, no floating point (`libm` unused), no QDOS traps or channel calls in the application. All QL-specific
  source is:
  * `main.c` (`#ifdef QDOS`): includes `<qdos.h> <qptr.h> <fcntl.h> <ansicondef.h>`, prints `QDOS!\n\n\n`, then
    `__ANSICONF__.emulation = VT100; __ANSICONF__.csi = 0; _conwrite = ANSI_conwrite;` — **all three symbols are
    referenced; none is defined in the archive** (they come from C68 libc / LibVT).
  * `termqd.c` (the QL terminal file): `char _prog_name[] = "Hack-1.03"`, `_cmdparams`, `_cmdchannels`,
    `_cmdwildcard`, `_stackchannels` = NULL (C68 start-up vectors), `struct WINDOWDEF _condetails = {2,1,0,6,512,256,0,0}`
    (C68's console window: red border width 1, paper 0, ink 6, 512×256 at 0,0), and a fixed ANSI termcap:
    `HO ESC[H  CL ESC[2J  CE ESC[K  UP ESC[A  CM ESC[%d;%dH  ND ESC[C  XD ESC[B  BC ESC[D  SO ESC[7m  SE ESC[0m
    TI ESC[0m ESC[=7l  TE ESC[0m ESC[=7h  CD ESC` (a bare ESC), `CO = COLNO (80)`, `LI = ROWNO (21)`.
  * `pctty.c`: `<conio.h>` only `#ifndef QDOS`; `readchar()` uses `getch()` after `fflush(stdout)`; `cgetret()` prints
    the debug text `standoutend is broken` (kept: the QL build shows it too).
  * `x68k.c`: `void signal(){}` and `void kill(int i){}` stubs.
  * `lev.c`: `#ifdef MSDOS && notdef QDOS` around a commented-out `setmode`.
* Platform configuration in effect (`config.h`, `hack.h`): `SYSV`, `MSDOS`; `UNIX`, `CHDIR`, `MULTIUSER_LOCK`, `MAIL`,
  `UNIX_SIGNALS`, `BSD` undefined; `NEWS "news"`, `RECORD "record"`, `HLOCK "perm"`, `HELP "help"`, `SHELP "hh"`,
  `DATAFILE "data"`, `RUMORFILE "rumors"`, `SAVEF "save/%d%s"` (uid, name), level files `<name>.<n>`, bones
  `bones_NN`, `O_BINARY` on save/bones, `rename` instead of `link`, `WIZARD "aeb"` (`-D` only for that name).
* Seeding: `setrandom()` = `srand((int) time(0))`; `rand()` from the libc. Wizard mode alone can reseed (`MAGIC=n…`).

## 3. Dependency summary

Full matrix (symbol → QL provider → QDOS/386 provider → adapter → status): `docs/R14A-QL-C-ABI.md` §2. In short:

* 59 externals from libraries (`build/r14a/hack/symbols.txt`): 52 are standard C / POSIX calls already in the
  QDOS/386 libc (stdio, string, malloc, `open/creat/read/write/lseek/close/unlink/fstat/getpid/getuid`, `time`,
  `localtime`, `qsort`, `getenv`, `signal`, `perror`); 3 are the C68 console vector (`_conwrite`, `ANSI_conwrite`,
  `__ANSICONF__`); 4 are C68 library calls the QDOS/386 libc lacks (`getch`, `getlogin`, `kill`, and `signal` which
  x68k.c also defines).
* Header-only gaps: `<ansicondef.h>`, `<qptr.h>`, the `struct WINDOWDEF` tag in `<qdos.h>`, `ESRCH` in `<errno.h>`
  (named by `unix.c:veryold()`, which is dead code because `MULTIUSER_LOCK` is undefined).

## 4. Licences and provenance

| Material | Stated terms | Redistributed in R14A |
| --- | --- | --- |
| Hack 1.0.3 source, data, help, hh, rumors, hack.6 | `copyright`: © 1985 Stichting Centrum voor Wiskunde en Informatica, BSD-style (retain notice; binary redistribution reproduces it; no endorsement) | yes (orig/ unchanged, notices kept) |
| Earlier Hack code | `copyright-jf`: © 1982 Jay Fenlason, BSD-style | yes |
| The QL/Windows/x68k port files and Makefiles | `LICENSE`: BSD-3-Clause, © 2022 neozeed | yes |
| QL binaries `hack`, `test/hack` | built from the above (BSD); the C68 libc 4.24.5 linked into them is © its authors (C68 is distributed freely; its terms are not restated in the archive) | yes, as the oracle, unchanged; not executed outside QLBox |
| `makedefs.exe`, `makedefs.obj` | built from `makedefs.c` (BSD) | yes, unchanged, never executed |
| `test/nethack`, `test/data`, `test/help`, `test/hh`, `test/rumors` | NetHack 1.0 material; **no licence stated in the archive**, no source for the binary | **no** — withheld (`orig-WITHHELD.txt`); never executed or used |
| QDOS/386 additions (`sdk/qlcompat/c68/c68vt.c`, `ansicondef.h`, `qptr.h`, the libc68 variant, `Makefile.qdos386`, scripts, tests) | written for R14A; same terms as QDOS/386 | yes |

The QDOS/386 `HACK.AOU` combines the BSD Hack sources with QDOS/386's own libc and shim; the BSD notices must travel
with it (the package carries `orig/copyright`, `copyright-jf`, `LICENSE`). A successful build is not taken as a
distribution right for anything the archive does not license (hence §4's withheld files).

SuperBASIC on the R13B/R14 example volumes remains private research (R14 rules): the R14A test volumes contain it and
are not published; the package ships the volume **scripts**, not images with SuperBASIC.

## 5. Patch proposal (Gate 0) and what was done

Proposed: no application-source change; an opt-in `Makefile.qdos386`; generic C68 interfaces in the shared layer.
Result after Stage 1: **0 changed lines in the Hack source** (`build/r14a/hack/source-changes.diff` is empty;
`third_party/hack103-ql/r14a/hack-qdos386.patch` does not exist). Build-side differences only:

1. `Makefile.qdos386` next to the unchanged `Makefile.qdos`: same object list minus `x68k.o` plus the shim `c68vt.o`;
   `-DQDOS -DMSDOS`; the QDOS/386 GCC 1.40; `onames.h` from `onames.z` without CR (GCC 1.40 rejects CR).
2. `x68k.o` is not linked: its `signal()` duplicates the libc's (two definitions do not link) and its `kill()` is
   provided by the shim with the same no-op effect.
3. `sdk/qlcompat`: `<ansicondef.h>` (C68 4.24.5 declarations), empty `<qptr.h>`, `struct WINDOWDEF` tag;
   `c68vt.c` (`ANSI_conwrite`, `__ANSICONF__`, `getch`, `getlogin`, `kill`).
4. `libc68.a`, linked before `libc.a`: the QDOS/386 `stdio.c` built with `-DQDOS_C68_CONWRITE` (C68's `_conwrite`
   vector, blocking console reads, C68's end message) and `posixio.c` built with `-DQDOS_C68_POSIX` (C68/Unix file
   semantics: full-count `read`, deleting an open file — both found by the gates); `libc.a` is byte-identical to R14's.
5. `<errno.h>`: `ESRCH 3` (a definition only; no binary changes — R14 binaries rebuild identically).

## 6. Oracle

The QL `hack/hack` runs under QLBox R20 (`--ram 640`, JS ROM) from a playground holding it and the source tree's
`data`, `help`, `hh`, `rumors`, an empty `perm` and `save/` (the `test/` NetHack text files are not used). QLBox's
QL clock follows the host, so QL dungeons are not repeatable, and C68's `rand()` is not the QDOS/386 libc's: the
comparison is of behaviour, text, layout and file handling, not of identical dungeons
(`docs/R14A-HACK-RUNTIME-REPORT.md`).
