# R12 Stage 4: SuperBASIC/C89 (SB4) on QDOS/386 — `BASIC.AOU`

> R12B note: the core now also has the host extension-name hooks (LOAD/SAVE/MERGE/DIR/EXEC...); see
> `docs/R12B-SUPERBASIC-FILE-ABI.md` §1 and `apps/superbasic/qdos386-core-changes.patch`. The text below describes R12.

Scope: the interpreter supplied as `SuperBASIC-C89-SB4.tar.gz` runs natively in the guest as `BASIC.AOU`, on its own virtual console, next to the editor and the shell. This is the **real SB4 core, unchanged**, cross-built with the historical GCC 1.40; only the operating-system-dependent layer (channels, console, stack) is new. It is a **subset** of SuperBASIC (see the supplied `KNOWN-LIMITATIONS.md`, copied to `apps/superbasic/docs/`); nothing here claims full SuperBASIC or QL compatibility.

Evidence classes: **host build** (host compile only), **guest executed** (QEMU full machine VGA/keyboard/IDE, `tests/r12/run-s4.sh`, `logs/r12/stage4/`), **independent comparison** (the supplied `.out` oracle files and the supplied host runner on the same files), **NOT TESTED**.

**Provenance.** SuperBASIC is private research, not freely redistributable. `apps/superbasic/docs/PROVENANCE-LICENSING.md` (as supplied) and `apps/superbasic/SHA256SUMS.sb4` (the supplied checksums) are kept. Short strings in `sb_error.c` (error texts) and `sb_lex.c` (keywords) overlap the ROM's text, as that document says. Nothing in this package is described as redistributable. There is no substitute interpreter: the core in `apps/superbasic/src` is the supplied one.

## 1. What changed relative to the supplied package

| Part | State |
|---|---|
| `src/sb_chan.[ch] sb_check.[ch] sb_error.[ch] sb_expr.[ch] sb_format.[ch] sb_interp.[ch] sb_lex.[ch] sb_mem.[ch] sb_store.[ch] sb_value.[ch]` | **byte-identical** to SB4 (`sh apps/superbasic/verify-core.sh` compares them with `SHA256SUMS.sb4`) |
| `src/sb_state.h` | **one edit**: the constant `SB_MAX_CALL_DEPTH` (5500, "hosted bound, not ROM behaviour") may now be a variable: `#ifdef SB_CALL_DEPTH_VAR` it expands to `extern int` of that name; without the define the file is the supplied one in effect |
| `sb_console.[ch]`, `sb_main.c` | **not used**: the hosted stdio glue and driver. Replaced by `qdos/` |
| `qdos/qb_io.c` | the QDOS channel adapters and the console line editor |
| `qdos/qb_stack.c` | the interpreter's private stack (heap block) and the call-depth limit |
| `qdos/basic.c` | `BASIC.AOU`: the interactive driver |
| `qdos/bastest.c` | `BASTEST.AOU`: the supplied `tests/sbtest.c` (unit checks and the corpus runner, same code) with a driver that reads manifests and writes a results file |

Build: `make -C sdk/examples BASIC.aout BASTEST.aout` (historical GCC 1.40, `-O`, slot 1, `-D SB_CALL_DEPTH_VAR=qb_depth_limit`). The core compiled unchanged with GCC 1.40 on the first attempt. BASIC.AOU is 80 KB.

## 2. Channels: SuperBASIC handles are not QDOS channels and not `FILE*`

The core reaches devices only through `SbChan` handles built from four callbacks. The port supplies:

| SuperBASIC | Adapter | Talks to |
|---|---|---|
| `#0 #1 #2` (read/write) | `con_write` / `con_read` | the job's own CON channel: output `qd_write` (≤ 200 bytes per call, partial writes looped by the core), input from the line editor |
| `#3` (write only) | `con_write` | the same console |
| diagnostics (error reports) | `diag_write` | the same console, bright red, attribute restored afterwards |

No QDOS channel is opened or closed by the adapters (the console channel is the job's own, closed by the system at exit). `OPEN`/`CLOSE`/`SAVE`/`LOAD` are not implemented in the supplied core ("not implemented", -19), so no SuperBASIC file handle maps to a QDOS file channel in this port; the file named on the command line is read by the driver with stdio and executed line by line.

## 3. Input and the console

The console is put in raw mode (no echo; cursor keys arrive as codes 0x80..0x99). `qb_getline()` is the shell editor's method: it draws from the cell where the line starts with absolute cursor moves, so lines wrap over several rows and the screen scrolls when the line reaches the bottom.

Keys: Left/Right/Home/End, Delete, Backspace, Up/Down (16-line history), Ctrl+A/E/B/F/P/N (same as Home/End/Left/Right/Up/Down), Ctrl+U or Esc (clear the line), Ctrl+K (delete to the end), Ctrl+C (abandon the line: nothing runs), Ctrl+D on an empty line (end of input → leaves BASIC). Typed lines are limited to 255 characters. `INPUT` uses the same editor and history. `QUIT`, `EXIT` or `BYE` alone on a line leaves (host-driver commands; they are not SuperBASIC). Kernels without the console key API (R11M) get the kernel's cooked line input (**NOT TESTED**).

**Break.** While a program writes to the console, `Ctrl+C` ends it with the ROM's "not complete" error; the other keys typed meanwhile are queued (64) for the next line read. A program that computes without printing cannot be interrupted this way: the core's own step safeguard (`SB_DEFAULT_STEP_LIMIT`, 1,000,000 control transfers per command, `/L:n` to change it) ends a runaway loop with "out of range", and the shell's `KILL /CON:n` ends the job.

## 4. The private stack — and the crash that shaped it

The core recurses in C: about **980 bytes per nested `DEFine` call** and **600 bytes per level of parentheses** (measured in the guest, GCC 1.40 `-O`). The job stack the kernel gives a job is 64 KiB, so the interpreter runs on its own, bigger stack, switched with a 10-line assembly helper.

**Finding (guest, triple fault).** The first version put that stack in the program's own BSS (640 KiB inside the 1 MiB paged window). `RUN BASIC.AOU` worked, because with the shell blocked there is nothing to switch to. The first `START BASIC.AOU /CON:2` killed the machine: QEMU's `-d int` trace shows the guest executing at the page-directory address (`EIP=0x122002`, `CR3=0x122000`) and double/triple-faulting. The scheduler loads the next job's page directory while it is still running on the interrupted job's stack, and a paged job's window is **private** — not mapped in any other directory. A stack there disappears at the first context switch. (Job stacks are kernel-heap blocks for exactly this reason.) Rule, now stated: *a paged job may switch its ESP only to memory of the shared kernel heap.*

Consequence: the stack is a `qd_alloc` block (default **256 KiB**, `/M:kib` 128..384; the next smaller size is tried when the heap cannot give it), painted and measured. The whole system shares one ~0.9 MB kernel heap, so this is a real cost (idle heap 667,024 bytes). The call-depth limit is derived from the size obtained: `(size − 100 KiB) / 1 KiB` = **156** nested calls at the default (the ROM reaches 5000+; the supplied corpus needs < 25). The 100 KiB headroom covers a typed line of 255 characters nested in parentheses (76 KiB), libc and the kernel's trap and interrupt frames. A guard of 16 words at the low end is checked at exit ("WARNING private stack overflowed"). Residual risk, stated: a pathological program that nests a long parenthesised expression inside every one of 150 call levels is not covered by that arithmetic.

Lines read from a file named on the command line are limited to the same 255 characters (longer ones are reported and skipped) so the stack budget holds. `BASTEST` runs the supplied corpus lines without that limit (they are short).

## 5. Evidence (Gate 4)

All **guest executed** unless stated; transcripts and judge output in `logs/r12/stage4/`; run with `SB4=... sh tests/r12/run-s4.sh OUTDIR` (four parallel QEMU runs) and `tests/r12/judge_s4.py`, `tests/r12/judge_s4_corpus.py`.

**Interpreter behaviour (judge_s4.py, 58/58).** Direct mode (`PRINT 2+3`, string concatenation with `&`, `a$`/`LEN`, the ROM's error texts "overflow", "error in expression"); numbered lines, `LIST` (the program as entered), `RUN` with `INPUT` (prompt, typed answer, computed result), `FOR`/`NEXT`, `DEFine FuNction` recursion (`fact(10) = 3.6288E6`, QL number format); an error inside a running program names its line ("overflow at line 10"); line editing (Home/Right/Delete/End, history Up + edit, Esc, Ctrl+C abandon, Backspace) and a 118-character line wrapped over two rows at the bottom of the screen, edited in the middle and executed correctly; `Ctrl+C` breaks a printing program ("not complete at line 10"), keys typed meanwhile are kept (type-ahead), the editor works afterwards; `QUIT` and `Ctrl+D` leave cleanly with exit status 0 and no live core blocks; `/L:20000` ends `10 GO TO 10` with "out of range at line 10"; `BASIC /Q DEMO.BAS` executes a file as typed, `INPUT` reading the keyboard; missing file (exit 2) and bad option (exit 1).

**With the other consoles (B).** Shell on console 0, `EDIT.AOU` on console 1 (`NOTES.TXT`), `BASIC.AOU` on console 2, all started with `START ... /CON:n`. BASIC waits at `INPUT`; text typed on console 1 goes to the editor only and BASIC's screen is untouched; after switching back, `Bob` answers the `INPUT`; the program (`FOR i=1 TO 4000` with an inner delay loop, ~10 s) then **keeps running while another console is visible**: `CONDUMP 2` from the shell shows it mid-run (`n=2000`), and it finishes with `done 3999`; the editor's text and cursor are intact after the round trips and `Ctrl+S` writes `NOTES.TXT` (11 bytes, read back from the volume with mtools: exactly `editor text`); `PS` shows both as paged jobs and then both `ENDED (status 0)`; `fsck.fat -n` clean; **independent**.

**Resources.** `LEAK.AOU` before/after: free heap 667,024, 15 free channel slots, 0 address spaces — identical after four batch launches, after three `START` + `KILL /CON:2` cycles with BASIC blocked in `INPUT` and a relaunch on the same console (42), and after the editor+BASIC session.

**Corpus (judge_s4_corpus.py, 19/19; `BASTEST.AOU`, the supplied runner's code).** The supplied unit checks pass in the guest (number formatting, error texts, canonical form, step limit, channel handles). **103/103** corpus programs and **3/3** examples produce byte-identical output to the supplied `.out` files with no core heap block leaked; the per-file result list is identical to the supplied host runner's on the same files (independent comparison, 0 differences). The **14 known-difference fixtures** (`tests/known`) were run with `/K`: **13 differ from the ROM oracle exactly as documented and as the host runner reports, 1 matches** — reported, not fixed (they are interpreter limitations listed in `apps/superbasic/docs/KNOWN-LIMITATIONS.md`, such as the ROM swallowing following input after a syntax error). Stack high-water over the corpus: 12,352 bytes.

**Limits observed (stated, not hidden).**

| Item | Value |
|---|---|
| Nested `DEFine` call depth | **156** at the default 256 KiB stack (`/M:128` → 28, `/M:999` → 284); depth 157 stops with "out of memory". The ROM reached 5000+ before running out of memory; the supplied corpus needs fewer than 25 (the same corpus passes on the host with the limit at 25). |
| 100 nested parentheses at the leaf of a 156-deep recursion | works; stack high-water ≈ 230 KB of the 256 KB block |
| Typed line / file line | 255 characters (a longer file line is reported and skipped) |
| Break | only while a program writes; otherwise `KILL /CON:n` or the step safeguard |
| Core heap | the shared kernel heap (idle 667 KB); a very large program ends with "out of memory" |
| Files, graphics, most built-ins | not implemented in the supplied core (`OPEN`/`CLOSE`/`SAVE`/`LOAD`, `CLS`, `INK`, `PRINT AT`…): error -19 / -17. `BASIC file.bas` is the way to run a program file |
| `INPUT "prompt";x` | not in the supplied core (use `PRINT "prompt";` then `INPUT x`); unchanged |

**NOT TESTED:** the cooked-input fallback on an R11M kernel (no console key API); BASIC on Tiny386 (only the single release smoke is planned there); exhaustion of the kernel heap by BASIC plus three other jobs; behaviour with the stack guard actually damaged (never observed); a program that nests long parenthesised expressions inside every call level.

## 6. How to use it

```
START BASIC.AOU /CON:2        run it on its own console (Alt+F3), or RUN BASIC.AOU on the shell's console
START BASIC.AOU DEMO.BAS /CON:2   execute DEMO.BAS first, then stay interactive (/Q: leave after the file)
```

Keys are in section 3. Edit a `.BAS` file with `EDIT` on another console and run it with `BASIC file.bas`.
