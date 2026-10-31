# R14A — LibVT / ANSI terminal for the QL Hack port

## 1. What Hack emits (source inspection, not guessed)

Every escape sequence comes from `termqd.c` (no other linked file writes ESC; `cmd.c`, `do_name.c`, `invent.c`,
`pctty.c`… only *read* ESC as a key). The terminal is fixed in `startup()`; `CO = 80`, `LI = ROWNO = 21`.

| Cap | Bytes | Used by | Parameters |
| --- | --- | --- | --- |
| TI | `ESC[0m ESC[=7l` | `start_screen()` | — (attributes off, auto-wrap off) |
| TE | `ESC[0m ESC[=7h` | `end_screen()` (`settty`, exit) | — |
| CL | `ESC[2J` | `cls()` — must also home the cursor | — |
| HO | `ESC[H` | `home()` | — |
| CM | `ESC[%d;%dH` | `curs()` via `tgoto(CM, x-1, y-1)` = `sprintf(.., y, x)` | row 1…24, column 1…80 |
| CE | `ESC[K` | `cl_end()` | — |
| UP / XD / ND / BC | `ESC[A` `ESC[B` `ESC[C` `ESC[D` | `curs()` short moves, `backsp()` | — |
| SO / SE | `ESC[7m` / `ESC[0m` | `standoutbeg/end()` (`--More--`, menus, `flags.standout`) | — |
| CD | `ESC` (a lone ESC) | `cl_eos()` from `docorner()` (menus taller than 22 rows) and the pager when not `whole_screen` | — |

Controls written: CR (`nocmov`), LF (newlines, `xputc('\n')`), BS (`getlin` erase echo `\b \b`), BEL (`bell()`), TAB
(text files shown by the pager — `help`, `hh`, `data` contain TABs). Characters: ASCII 0x20–0x7E only (map glyphs,
`objclass.h`/`rm.h` symbols, messages); nothing above 0x7E, no 0x7F. Since `LI - ROWNO - 2 <= PAGMIN`, `whole_screen`
is TRUE, so the pager clears the screen (CL) instead of using CD; CD is reached only by `docorner()` for very long
menus, where the next bytes are a CM (`curs`) — the lone ESC therefore swallows the ESC of that CM's own sequence
start… which a VT100 parser treats as the start of a new sequence (§3).

## 2. The C68 interface (C68 libc 4.24.5, the library in the QL binary)

From the C68 libc 4.24.5 source (`include/ansicondef.h`, `include/fcntl.h`, `fcntl/_conwrite.c`, `lattice/getch.c`,
`defaults/libvt.s`):

* `typedef struct { char emulation; char csi; } ANSICONDEF; extern ANSICONDEF __ANSICONF__;` with the bits
  `VT52 0x01, VT100 0x02, ANSI1..ANSI4 0x04..0x20`; `extern int ANSI_conwrite();`
* `int (*_conwrite)(struct UFB *, void *, unsigned int)` — called by `write()` for console channels; result `> 0`:
  that many bytes may be written unchanged, `< 0`: that many were consumed, `0`: everything done. The default
  (`default_conwrite`) handles `\b \r \a \f \t` and passes text through.
* `getch()` (Lattice compatible): one byte from the console channel, no echo, no flush, waits.
* `libvt.s` in the libc is a dummy (`LIBVT_VER`) — **the real LibVT was not supplied**, so its exact rendering of
  each sequence is unknown; the oracle is the QL Hack binary's screen under QLBox.

## 3. Chosen architecture — option A (userspace shim)

```
Hack (unchanged)  --stdio-->  libc68.a stream_flush  --C68 _conwrite protocol-->  ANSI_conwrite (c68vt.c)
                                                                                   | qd_con_goto / qd_con_attr / qd_write
                                                                                   v
                                                         the process's own CON session (kernel cells + cursor)
```

* **libc68.a** = the QDOS/386 `stdio.c` compiled with `-DQDOS_C68_CONWRITE`, linked before `libc.a`. It defines
  `_conwrite` (default NULL = plain console) and, when a program sets it, offers console text of stdout/stderr with
  C68's protocol and a `struct UFB` (C68's leading fields, `ufbtyp = D_CON`, `ufbfh` = CON channel). Its console
  read waits in the kernel (`qd_con_getkey`; ENTER 13 → 10). `libc.a` is byte-identical to R14's.
* **c68vt.c** (`sdk/qlcompat/c68/`): `__ANSICONF__ = {VT100, 0}`, `ANSI_conwrite`, `getch`, `getlogin`, `kill`.
  The parser state (mode, parameters, cursor, attributes, wrap) is per process (static data in the program's own
  address space), survives any split of the byte stream and is never shared. Output is batched into runs and drawn
  with the existing R11M/R12 console calls on the caller's session; the kernel console interprets no escapes, so a
  sequence can never leak to another task. No kernel change: R14A runs on the unchanged R14 kernel.
* Not chosen: option B (a kernel parser) — it would change the kernel and every console for one application.

Interpreted: CSI `H f A B C D J(0,1,2) K(0,1,2) m s u`, `ESC[=7h/l` and `ESC[?7h/l` (auto-wrap), `ESC 7 8 D M E c`,
CR, LF (column 0 + next row, scrolling the whole screen at row 24 as the QDOS console does), BS, TAB (8), BEL
(no sound: nothing shown). SGR `0 1 4 5 7 22 24 25 27 30–37 39 40–47 49`; 0 restores the measured QL look **green on
black** (attribute 0x02); bold/underline → bright, reverse swaps. Bounds: at most 8 parameters, each capped at 9999,
positions clamped to 25×80; CAN/SUB abort a sequence; a byte ≥ 0x80 inside a CSI drops it; any unknown final byte or
ESC + character is consumed silently. A lone ESC consumes the next byte as its final byte; when that byte is ESC it
starts a new sequence (so Hack's `CD` followed by a `CM` positions correctly). `emulation` without VT100/ANSI bits:
`ANSI_conwrite` returns `n` and the bytes go to the console unchanged (VT52 alone is not emulated — Hack does not use it).

## 4. Glyphs and character set

The QL font and code page 437 agree on 0x20–0x7E except: **0x60** (`` ` ``, Hack's boulder/rock `ROCK_SYM`) is a
pound sign £ in the QL character set and a grave accent in CP437 — shown as `` ` `` (documented fallback; no
translation, so files and messages keep their bytes); **0x5E** `^` (traps) and **0x7C** `|` (walls) are the same
codes (the VGA ROM draws `|` with a small gap); **0x7F** (QL ©) is never emitted. No character above 0x7E is emitted.
Width: the QL window (`_condetails` 512×256, 85 columns of 6-pixel characters) shows the same 80-column layout
because Hack positions everything with CM inside 80×24; QDOS/386 has 80×25 cells. `_condetails` (border, colours,
size) is not applied: the session is the whole screen, black paper, green ink by default.

## 5. Tests (Gate 2, `tests/r14a/run-gate2.sh`, logs in `logs/r14a/gate2/`)

* **Host** (`tests/r14a/host-c68vt.c`, 49 checks): `ANSI_conwrite` on a model of the QDOS/386 console (output rules
  of `docs/R11M-CONSOLE-ABI.md`): every Hack capability at four split sizes (whole, 1, 2, 3 bytes per call), clamping,
  18 parameters, huge numbers, malformed/CAN/SUB, CD + CM, split across calls, save/restore (two forms), BS CR TAB BEL,
  wrap off/on, scrolling at row 24, SGR colours, the default green-on-black, emulation off → `n`, 200 000 random bytes
  never move the cursor off-screen. Mutation check: breaking the wrap column, `J` or malformed-byte handling makes it
  fail (47/49, 41/49, 48/49).
* **Guest** (QEMU, R14 kernel): `ANSIT.AOU` (`tests/r14a/ansit.c`, linked exactly like Hack) draws every Hack
  sequence (half of them one byte per write) in two sessions A and B; exact screens, attributes (`%cells`) and
  cursors; keys go only to the foreground session; three rounds of switching through the Task Manager, Alt+F1 and
  `CON n` restore each screen, attributes and cursor exactly; quitting shows TE, the last line and C68's end message
  "Press a key to exit" in the program's own session, the key releases the session, and the kernel heap, channels
  and address spaces return to their starting values (30 checks with the host test). Hack's own screens are compared with the QL build in Stage 3.
