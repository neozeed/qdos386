# R12 Phase 0 audit

Branch `r12-interactive`, created from the frozen R11M-FIX1 commit `6fbb9ff7c8a695d91a368a985a2b1da8b53a9bbc`. R11M parent `a16edcd` stays
frozen. Claim classes: **host build** / **guest executed** / **independent comparison** / **not tested** / **blocked**.

## 1. Baseline verification

| Check | Result | Class |
|---|---|---|
| `qdos386-R11M-FIX1-TINY386-SHUTDOWN.tar.gz` SHA256 | `e6fb5e7b0433b2167dd4cfcd20f44278760af8152a90e96aef1a9d30f4ffab14` (matches the handoff) | independent comparison |
| `SHA256SUMS` inside the archive | 2282 of 2282 entries OK, 0 differ | independent comparison |
| working tree HEAD | `6fbb9ff`, clean except untracked `logs/` evidence | host build |
| QEMU smoke (`R11M_QKERNEL=build/r11m/pc-pg-vga`, FIX1 demo volume): `PWD`, `MD SRC`, `CD SRC`, `COPY` to a VFAT long name, `DIR`, `TYPE` of the long name, `START COUNTER`/`TICKER` on consoles 1 and 2, `PS`, Alt+F2/Alt+F1, `CONDUMP 1`, `KILL`, `RUN PHOON.AOU`, `EXIT` | all steps ran; 0 `PROMPT-TIMEOUT`/`EXPECT-TIMEOUT`; final image `fsck.fat` clean, FAT copies equal, `tools/r11m-fatwalk.py --strict --vfat` 67 files 1 directory 0 errors | guest executed, independent comparison |
| SuperBASIC SB4 archive `SHA256SUMS` | all files OK | independent comparison |
| SuperBASIC host build and corpus (`scripts/build.sh`, `scripts/test.sh`, gcc) | 106 corpus files + 14 examples/unit checks pass, 0 failing; 14 `tests/known` fixtures reported KNOWN | host build |

## 2. What the R11M code gives R12 (audited from the source, not from the docs)

### 2.1 Keyboard decoding (`kernel/kbd.c`)
Scancode set 1 decoder with Shift and (VCON builds) Alt. Alt+F1..F4 and Alt+1..4 return code 2 (a console hotkey). The `E0`-prefixed keys are **dropped**
except keypad Enter and right Alt. There is **no arrow, Home/End, PageUp/PageDown, Delete, Insert, Esc, Ctrl or function key**: nothing an editor needs reaches
a program. A byte above 0x7F never occurs on the input path today, so the range `0x80..0xFF` is free for key codes (decision, section 6.1).

### 2.2 Console cells, cursor, input (`kernel/vcon.c`, `vcon.h`)
Four 80x25 consoles, each with its own cell array (char + attribute), cursor (a column may equal 80 = lazy wrap pending), attribute, scroll region, 127-byte
input queue, 120-byte cooked line, one waiter. Raw mode delivers bytes as typed; `QD_CON_ECHO` echoes printable bytes only (bytes above 0x7E are never echoed);
`QD_CON_COOKED` accepts only 32..126 and tab and drops everything else, so extended key codes cannot leak into a cooked line. Output ignores control bytes
other than CR LF BS TAB. The cursor query returns column 80 while a wrap is pending (the shell editor must handle that). Existing logical API: goto, clear
(ALL/EOL/LINE/EOS), attribute, size, cursor query, cursor visibility, mode, region, number, focus, child, snap, diag (`docs/R11M-CONSOLE-ABI.md`). **Missing for
an editor:** key codes, an API version/capability query, a non-blocking "queued byte count" wrapper (the kernel query `VQ_QUEUED` exists, no library call).

### 2.3 Shell (`apps/shell/shell.c`, `shell_r11m.inc`, `shell_jobs.inc`)
Application in slot 0 (256 KiB window, binary 16.8 KiB), built with `-DR11M_SHELL -DR11M_JOBS -DSH_LINE_MAX=160` using raw `qtrap` (`appcalls.h`), no libc.
* `read_line`: byte at a time via `CH_READ` + `QT_CON_WAIT`, echo by the shell, **BS/DEL only**; printable 32..126; no cursor keys, no history.
* `exec_line`: the command word is the text up to the first blank, upper-cased; arguments keep case; trailing blanks trimmed. Quoting is handled per command: `tok()`
  splits blank-separated words keeping quoted runs, `uq()` removes quotes. `DIR` takes the **whole remaining text as one path** (`cmd_dir_m(uq(arg))`): `DIR /W`
  would today try to list a directory called `/W`.
* `DIR`: opens the directory with `OPEN_READ|OPEN_DIRS`, switches to 96-byte long records with `CH_CTL` op 5 (falls back to 16-byte records), prints
  `ALIAS  SIZE|<DIR>  Longname`, then `N files, M directories`. No width handling, no pause.
* Jobs: `START/PS/WAIT/KILL/CON/CONDUMP/DIAG/SLEEP` use the tagged-id job table (`jt[SJ_MAX]`, 8 entries).

### 2.4 Jobs, ownership, paging (`kernel/jobs.c`, `jobctl.c`, `paging.c`, `aout.c`)
`JOB_MAX` 6 (SYSTEM, IDLE, shell + **3**), `PG_PROC_MAX` 3 address-space records, application job stack 64 KiB on R11M kernels (`-DQDOS_APP_STACK=65536u`),
kernel heap arena ~0.9 MB shared by every job (BSS end .. 0x1FFFFF). Each paged job gets a private 1 MiB image window at 0x240000 (text+data+BSS, so a
static 256 KiB buffer in BSS is private and costs no heap). `KILL` ownership is unchecked; a paged job cannot start or kill. **Finding (affects the Stage 3
demo):** the PC/QEMU frame pool is `APP_PROFILE2` + `APP_PROFILE3` = 0x380000..0x47FFFF and 0x500000..0x5FFFFF, i.e. **2 windows**, although the job table has room
for 3 paged jobs. Editor + ticker + Phoon (or EDIT + BASIC + COUNTER) needs 3. Plan: an opt-in R12 map option (`QDOS_PG_WIN3`) that makes profile 3 two MiB
(0x500000..0x6FFFFF, still below the boot stack at 0x7FFFF0), giving 3 windows on QEMU and Tiny386 (both have at least 8 MiB). Default images unchanged.

### 2.5 Channel and FS APIs
`qd_open_ex` with native paths (`WIN1:/DIR/NAME`, relative to the job's cwd, quoted VFAT long names up to 63 ASCII characters), `qd_write` (<= 256 bytes per
call), `qd_flush`, `qd_truncate`, `qd_fsize`, `qd_remove`, `qd_rename`, `qd_stat`, `qd_getcwd`, `qd_mkdir`. Writes are write-through; the directory entry is written at
flush/close/job exit. **`qd_rename` onto an existing name fails with -83**, so a replace-by-rename needs a delete of the target between two steps: a save can be made
failure-safe against write errors (the new data is complete before the original is touched) but **not** atomic across a power cut (Stage 2 policy).

### 2.6 Libraries available to a C89 port
`sdk/libc`: ctype, errno, stdio (buffered), stdlib (malloc over `qd_alloc`, qsort, ...), string, `strtod` (own implementation, `fpbig.c`), time, `scanf`,
setjmp. `sdk/libm`: ceil, fabs, floor, frexp, ldexp, modf, pow, sqrt, trans. x87 FPU is enabled. `qd_con_channel()` gives the CON channel.

## 3. SuperBASIC (supplied archive `SuperBASIC-C89-SB4.tar.gz`)

Inspected, not assumed: `README.md`, `docs/{KNOWN-LIMITATIONS,TEST-RESULTS,ARCHITECTURE,SB3-CHANNEL-ABI,SB4-QDOS386-READINESS,PROVENANCE-LICENSING}.md`, all of `src/`, `tests/sbtest.c`.

* **Revision: SB4.** Strict ISO C89 hosted interpreter, 6387 lines in `src/`. Implemented (from the README and confirmed by the corpus): numeric/string literals, ROM
  operator precedence, `&`, comparisons, AND/OR/XOR/NOT, DIV/MOD, variables, LET, PRINT (`;` `,` `#n`), numbered lines, LIST (`#n`), RUN, RUN n, GOTO, IF..THEN,
  FOR/NEXT/STEP, STOP, CONTINUE, NEW, CLEAR, REM, LEN, ABS, ERNUM, INPUT (`#n`), GOSUB/RETURN, DATA/READ/RESTORE, DIM (numeric arrays any rank, one-subscript string
  arrays, slices), channels 0..3 and, new in SB4, `DEFine PROCedure/FuNction`, `END DEFine`, `RETurn`, `LOCal`, parameters and recursion.
* **Not implemented:** two-subscript string arrays, nested DEFine, graphics/screen words, files (`OPEN`/`CLOSE`/`SAVE`/`LOAD`), most built-in functions. No QDOS/386 backend
  exists; `sb_console.c` and `sb_main.c` are hosted stdio glue.
* **Channel interface** (`src/sb_chan.h`): `SbChanOps { write(obj,s,n,&done); read(obj,buf,n,&got); flush; close }`, flags `SB_CH_READ`/`SB_CH_WRITE`, handles 0..3 plus
  a diagnostic handle for error reports. Errors -6 not open, -19 not implemented, -40 I/O. EOF before any byte of a line is -19 for INPUT. The QDOS adapter replaces
  `sb_console.c`/`sb_main.c`; the core (`sb_interp`, `sb_expr`, `sb_lex`, `sb_store`, `sb_value`, `sb_format`, `sb_check`, `sb_error`, `sb_chan`, `sb_mem`) stays unchanged.
* **Allocator** (`sb_mem.c`): `SbAlloc` function table, default `malloc`/`free`/`realloc`; the test runner checks that every block is released.
* **Libc/libm surface:** malloc/free/realloc, memcpy/memset/memcmp/strlen/strchr, `strtod`, `sprintf`, `frexp`, `ldexp`, `floor`, `pow`, ctype. All present in the SDK.
* **Float:** C `double`, so x87 on QDOS/386. The ROM's wider exponent range is a documented difference (`KNOWN-LIMITATIONS.md` section 3) and does not change.
* **Error codes:** -1..-19 as in the ROM tables in `sb_error.c`.
* **Known ROM divergences:** 14 fixtures in `tests/known/` (syntax swallow, bare DATA words, `range_power`, concatenated text in arithmetic, ...). They are reported, never fixed
  or hidden.
* **C stack:** procedure and function calls recurse on the C stack, about 1.1..1.5 KB per level, `SB_MAX_CALL_DEPTH` 5500. A 64 KiB job stack holds about 40 levels. The port
  must (a) run the interpreter on a larger stack it allocates, or (b) lower the limit so that -3 (out of memory) is reported before the stack ends. Decision in section 6.4.
* **Provenance:** private research, not freely redistributable (`docs/PROVENANCE-LICENSING.md`): the error message texts in `sb_error.c` and the keyword spellings in `sb_lex.c`
  overlap the ROM text. The R12 package keeps this notice and the SB4 `SHA256SUMS`, and says it is for the owner's private use. No substitute interpreter is built.

## 4. Gaps found that the stages must close

| # | Gap | Stage |
|---|---|---|
| G1 | no cursor/edit keys, Esc, Ctrl, F-keys from the PC keyboard | 1 |
| G2 | shell has no line editing or history | 1 |
| G3 | `DIR` takes switches as part of the path; no `/W`, no `/P` | 1 |
| G4 | no API version/capability query; `VQ_QUEUED` has no library call | 1 |
| G5 | `qd_rename` cannot replace an existing file | 2 (save policy) |
| G6 | 2 frame windows although the job table allows 3 paged jobs | 3 |
| G7 | `START` has no usage hints; `PS` columns are numeric only; `CON` prints no help | 3 |
| G8 | a paged job's stack is 64 KiB; SuperBASIC recursion needs a bounded depth | 4 |
| G9 | no guest runner for the SuperBASIC corpus | 4 |

## 5. Stage dependency map

```
Phase 0 audit
   |
Stage 1  kbd.c extended keys --+--> console API v2 (libqdos) --+--> shell DIR /W /P, line editor, history
   |                           |                               |
   |                           +-------------------------------+--> demo program CONDEMO (positions/clears on its console)
   v
Stage 2  EDIT.AOU (needs: key codes, qd_con_*, qd_open_ex/qd_write/qd_flush/qd_rename, 256 KiB BSS buffer)
   v
Stage 3  QDOS_PG_WIN3 (3 windows) + shell job ergonomics (needs: START of EDIT/COUNTER/PHOON on consoles 1..3)
   v
Stage 4  BASIC.AOU = unchanged SB4 core + QDOS channel adapter (needs: raw console keys, line input from EDIT-style loop, 3 windows for the integrated demo)
   v
Stage 5  (optional) graphics surface proof, only if Gates 1..4 are stable
   v
Release  one QEMU matrix, one Tiny386 smoke (0x8900 shutdown), docs, package
```

## 6. Design decisions taken from the audit

1. **Key codes.** Special keys arrive as **single bytes 0x80..0x9F** in raw mode: `0x80` Up, `0x81` Down, `0x82` Left, `0x83` Right, `0x84` Home, `0x85` End,
   `0x86` PageUp, `0x87` PageDown, `0x88` Delete, `0x89` Insert, `0x90..0x99` F1..F10; Esc is `0x1B`; Ctrl+letter is `0x01..0x1A` (Ctrl+H/I/M/J coincide with BS/Tab/CR/LF, as
   on every terminal). Cooked mode drops them (it already accepts only 32..126). Decoding is compiled only with `-DQDOS_VCON`, so default images stay byte-identical.
2. **API version.** `CH_CTL` op `0x1B` returns `(version << 16) | feature bits`; the console API becomes version 2 (R11M was implicitly version 1). `qd_con_version()` returns
   `QD_ERR_NOTSUP` on an older kernel.
3. **Shell redraw** is based on the start cell of the line (queried once) and re-derived after every redraw from the real cursor, so scrolling and the pending-wrap column 80 are
   handled by measurement, not by assumption.
4. **SuperBASIC stack:** the port allocates its own interpreter stack and switches to it (the job stack is ordinary heap and the kernel runs traps on whatever stack the
   caller has); if that proves unsafe on QEMU the call-depth limit is lowered and the difference is reported.
5. **Tests:** QEMU primary; one Tiny386 smoke at the end; no guest GCC, no i386VM.
