# R12 console API version 2 and the EDIT.AOU editor

Scope: the logical text-console interface that R12 adds on top of the R11M virtual consoles, the raw key codes, and the editor (`EDIT.AOU`) that is built only on them. Applications never touch VGA memory or the keyboard controller; the debug port 0x402 is separate from application screen output (`qd_con_diag`).

Evidence classes used below: **host** (host build/test), **guest** (executed in QDOS/386 on QEMU full machine VGA/keyboard/IDE), **independent** (guest result compared with mtools / `fsck.fat` / `tools/r11m-fatwalk.py --vfat`), **NOT TESTED**.

## 1. Console API (libqdos, `sdk/include/qdos386.h`)

Every call takes a CON channel (`qd_open("CON")`, or the job's own console channel) and acts on the console that channel is bound to. **Coordinates are 0-based**: row 0 is the top line, column 0 the left column; the console is 25 rows by 80 columns. Return values: `>= 0` success, `QD_ERR_NOTSUP` (-15) on a kernel without the console option, `QD_ERR_RANGE` (-14) for an argument out of range.

| Call | Meaning |
|---|---|
| `qd_con_goto(ch,row,col)` | absolute cursor move, clamped to the console |
| `qd_con_gotoxy1(ch,row,col)` | the same, 1-based ANSI order (for ported code) |
| `qd_con_cursor(ch,&row,&col)` | cursor position; a pending-wrap cursor (column 80) is reported as 79 |
| `qd_con_clear(ch,what)` | `QD_CLR_ALL` (cursor home), `EOL`, `LINE`, `EOS` |
| `qd_con_attr(ch,attr)` | attribute for new text, `QD_ATTR(fg,bg)`; returns the old one |
| `qd_con_show_cursor(ch,0/1)` | hardware cursor visibility |
| `qd_con_size(ch,&r,&c)` | 25 x 80 |
| `qd_con_mode(ch,flags)` | `QD_CON_ECHO`, `QD_CON_COOKED`; 0 = raw, no echo (the default); returns the old flags |
| `qd_con_region(ch,top,bottom)` | scroll region |
| `qd_con_number / is_foreground / focus / child / snap` | R11M, unchanged |
| **R12** `qd_con_version(ch)` | console API version: 2 on R12 kernels, `QD_ERR_NOTSUP` (-15) on R11M |
| **R12** `qd_con_caps(ch)` | `QD_CONCAP_KEYS` (1: the input device delivers the key codes below), `QD_CONCAP_QUEUED` (2) |
| **R12** `qd_con_queued(ch)` | bytes waiting in this console's input queue (0..127) |
| **R12** `qd_con_getkey(ch)` | blocking read of one key code (the same call as `qd_con_getc`); `QD_EOF` at end of input |
| **R12** `qd_con_putat(ch,row,col,s,n)` | goto + write n bytes; bytes written, or `QD_ERR_RANGE` for a row/column/length outside the screen |

Kernel side: `CHAN_CON_CAPS` (0x1B) in the console control channel returns `(version << 16) | features`; the PC keyboard driver sets the KEYS feature. A kernel without it answers -15 and `qd_con_version` reports that; the shell then uses its plain line reader.

Behaviour that applications can rely on (guest, QEMU): `\n` on the last row scrolls the console; writing to column 79 leaves the cursor in a pending-wrap state (the next printable byte wraps; `qd_con_cursor` reports column 79); tab advances to the next multiple of 8; CR returns to column 0; each console has its own cursor, attribute and contents (`CONAPI.AOU`, 44 checks; `CONDEMO.AOU` + `CONDUMP` showing console 1 unchanged while console 0 is used).

## 2. Key codes (raw mode)

On a PC keyboard in raw mode (`qd_con_mode(ch,0)`) every key is one byte:

| Key | Code | Key | Code |
|---|---|---|---|
| Up / Down | 0x80 / 0x81 | F1..F10 | 0x90..0x99 |
| Left / Right | 0x82 / 0x83 | Esc | 0x1B |
| Home / End | 0x84 / 0x85 | Enter | 0x0D |
| PgUp / PgDn | 0x86 / 0x87 | Backspace | 0x08 |
| Delete / Insert | 0x88 / 0x89 | Tab | 0x09 |
| Ctrl+letter | 0x01..0x1A (`QD_KEY_CTRL('S')` = 0x13) | printable | the ASCII code |

Ctrl+H, Ctrl+I, Ctrl+J and Ctrl+M coincide with Backspace, Tab, LF and Enter, as on every terminal. **Cooked mode drops the codes 0x80..0x99** and echo never shows them, so line-oriented programs are unaffected. **Alt+F1..F4 and Alt+1..4 switch consoles and never reach a program**; the keys always go to the foreground console's input queue only (127 bytes; further keys are dropped). The decoder exists only in kernels built with `-DQDOS_VCON`; default images are byte-identical to R11M-FIX1 (six hashes, `scripts/r11l-default-check.sh`).

Evidence: host `tests/host/r12/run.sh` (scan-code decoder, 39 checks VCON build, 8 default build); guest `CONDEMO.AOU` reports 17 distinct keys typed on console 1, in order, with no leakage to the shell on console 0 (judge_s1, QEMU).

## 3. EDIT.AOU

```
EDIT [path]
```

An ordinary slot-1 (paged) application. It needs the 80x25 console; otherwise it prints a message and exits 1. A path is relative to the job's current directory, or absolute (`WIN1:/SRC/HELLO.C` or `/SRC/HELLO.C`); a name with blanks is quoted at the shell (`EDIT "WIN1:/SRC/My File.txt"`) or typed after Ctrl+S in an untitled buffer. The path is limited to 64 characters. A name that does not exist opens an empty buffer ("new file"); the file is created on the first save.

Keys: arrows, Home/End, PgUp/PgDn, F2 (top), F3 (end), printable, Tab (inserted as a tab, shown to the next multiple of 8), Enter (split), Backspace/Delete (join at line ends), Ctrl+S save, Ctrl+Q or F10 quit, Ctrl+K cut line (repeated cuts collect), Ctrl+U paste, Ctrl+F find, Ctrl+N or F4 find next, Ctrl+G go to line, Ctrl+L redraw, F1 help. The status line (row 24) shows the name (`..` + the tail if longer than 34), `[+]` when modified, line, column (visual, tabs expanded), `LF`/`CRLF`; errors are shown there in white on red.

Limits and policy (stated, not hidden):

* The text lives in one gap buffer of 262144 bytes in the program's own BSS: nothing comes from the shared kernel heap. A file over 262144 bytes is **refused, not truncated** (message, exit status 2). A file of exactly 262144 bytes opens; typing then answers "buffer full".
* A file containing a NUL byte is refused as binary (exit 2). All other bytes are kept exactly; control and non-ASCII bytes are shown as `.`.
* **Line ends.** If every line ends in CR LF the file is edited as LF and saved as CR LF; any other file is kept byte for byte (a lone CR is an ordinary character). A missing final newline stays missing.
* Lines of any length (up to the buffer) scroll horizontally; files longer than a screen scroll vertically.
* **Save.** The editor opens no channel except during load and save, so another job may rename or delete the file meanwhile. Save writes the complete text to `~EDIT.TMP` in the target's directory, flushes and closes it, renames an existing target to `~EDIT.BAK`, renames the temporary file to the target and removes the backup. On any error before the swap (path invalid, directory missing, volume full, write failure, write-protected) the original is untouched and the temporary file is removed; a failed rename puts the original back. A leftover `~EDIT.TMP` / `~EDIT.BAK` after a kill or power loss is a complete copy. **This is not atomic across a power cut or a kill between the two renames; no crash-atomicity is claimed.** A failed "Save as" in an untitled buffer leaves it untitled, so Ctrl+S asks again.
* Quitting with unsaved changes needs Ctrl+Q a second time; any other key after the warning goes on editing (and is inserted).
* No undo, no auto-indent, no mouse, no regular expressions; find is a case-sensitive substring search that wraps around.

Exit status: 0 normal; 1 usage / not an 80x25 console; 2 file refused or unreadable; 3 end of input.

### Evidence (QEMU full machine, scripted keyboard, `tests/r12/run-s2.sh`; results in `logs/r12/stage2/`)

See `docs/R12-RUNTIME-REPORT.md` for the final tally. Classes: create/edit/save/switch/exit on console 1, reopen after a fresh boot, CRLF/LF/no-final-newline/empty/long-line/tab/100-line/exactly-full/oversized/binary files, missing directory, volume full, long-name file saved through the name prompt, find/cut/paste/go-to/join: **guest**; byte comparison of every resulting file, `fsck.fat -n` and the VFAT walker on all four volumes: **independent**; heap/channel/address-space probe (`LEAK.AOU`) identical before and after repeated edit sessions: **guest**. **NOT TESTED:** a write-protected volume on QEMU (QEMU 8.2's IDE disk cannot be attached read-only, "Block node is read-only"; the editor's error path for it is the same code as the volume-full path, and the kernel's -80 handling was tested in R11L on the retired CPU-only harness); power-cut behaviour; files on a second volume.
