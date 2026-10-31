# R12 shell: DIR /W, DIR /P, line editing, history

Applies to the R11M-style shell (`apps/shell/shell.c` built with `-DR11M_SHELL -DR11M_JOBS`, SHELL.AOU on the volume) running on a kernel with the R12 console API
(`CHAN_CON_CAPS` version 2). On a kernel without it the shell falls back to the plain R8 `read_line` (BS/DEL only). The shell is application code: no kernel change is
needed for `DIR` itself; the editor needs the key codes of `docs/R12-CONSOLE-EDITOR-ABI.md`.

## 1. `DIR [path] [/W] [/P]`

Switches and the path may come in any order (`DIR /W path`, `DIR path /W`, `DIR /W /P path`). **Switch rule:** an unquoted token that is `/` plus exactly one character is a
switch. `/W` and `/P` (either case) are accepted; anything else of that shape (`/X`, `/A`) answers `unknown switch /X (DIR accepts /W and /P)` and lists nothing. A
quoted token is never a switch, so `DIR "/W"` lists a directory called `/W`. A path that is a slash and one letter (a root directory `/A`) must therefore be written
`WIN1:/A` or `"/A"`. Longer slash paths (`/BIN`, `/SRC/X`) are paths. More than one path gives the usage line. Quotes go around a name with blanks
(`DIR "Long Directory Name" /W`).

### Plain listing (unchanged format)
`ALIAS<pad to 14>SIZE|<DIR>  Long name`, header ` Directory of <path>`, footer `N files, M directories`. The long name is shown only when the entry has one.

### `/W` wide listing
* Names are listed **row-major** in columns (DOS `/W` order = directory order), one header line and one footer line as above.
* **VFAT choice:** the entry's **long name** is shown (the 8.3 alias only when the entry has no long name or a long name this kernel cannot represent). The alias is not
  shown in wide mode; use plain `DIR` for it.
* A subdirectory is shown as `[Name]`.
* The column width is the longest displayed name (brackets included) + 2; the number of columns is `80 / width` (at least 1). A long name is at most 63 characters, so
  the width never exceeds 67 and **a name is never truncated**; with such names the listing is one column. The last column of a row is not padded, so no row is 80 wide
  followed by a wrap.
* Two passes over the directory (the first measures), so the listing is correct for any entry count.

### `/P` pagination
* A page is **23 lines** (the console has 25 rows: the command line stays visible, one row is the prompt).
* The pause prompt `-- More -- SPACE next page, ENTER next line, Q stop` is shown **only when another line follows**, so a listing of exactly 23 lines (header + entries +
  footer) does not pause, and one more line does. An empty directory (`0 files, 0 directories`) never pauses.
* The key is read from **the shell's own console** (its input queue), so another console's typing never answers the prompt. SPACE (or any other key) shows the next page,
  ENTER shows one more line, `Q`, `q`, Esc or Ctrl+C stops the listing (no footer is printed; the prompt returns). End of input also stops it.
* While the prompt is displayed, console switching (Alt+Fn) works as always; the shell keeps waiting on its own console.
* `TYPE` is not paginated.

## 2. Line editor

| Key | Action |
|---|---|
| printable 0x20..0x7E | insert at the cursor |
| Left, Right / Ctrl+B, Ctrl+F | move one character |
| Home, End / Ctrl+A, Ctrl+E | start / end of the line |
| Backspace (0x08/0x7F) | delete before the cursor |
| Delete / Ctrl+D | delete under the cursor |
| Up, Down / Ctrl+P, Ctrl+N | previous / next history entry (the line being typed is restored after the newest) |
| Ctrl+U, Esc | delete the whole line |
| Ctrl+K | delete from the cursor to the end |
| Ctrl+C | abandon the line (`^C`, new prompt) |
| Enter | run the line (the cursor may be anywhere) |
| other keys (Tab, Ins, PgUp, PgDn, F-keys) | ignored |

* **Limit:** the line holds at most `SH_LINE_MAX - 1` = **159** characters. A further printable key is rejected (ignored, no beep). The 159-character line runs normally.
* **Redraw:** the editor remembers the cell where the line starts (the cursor after the prompt), draws with absolute cursor moves, and clears to the end of the screen after
  every change, so a line that wraps over several rows is edited correctly in the middle. When the end of the line would pass the last row the screen is scrolled up
  first (the start row moves with it). Appending at the end writes only the new character.
* **History:** the last `HIST_N` = 16 distinct consecutive commands, **volatile and per shell**. A line is stored exactly as typed (quotes included) before it runs;
  empty lines and a repeat of the newest entry are not stored. `HISTORY` lists them numbered.
* The history, cursor and line live in the shell's own static memory (slot 0). No kernel state is involved.

## 3. Evidence

| Evidence | Class |
|---|---|
| `tests/r12/seq-s1-dir.txt` on QEMU (VGA text capture, monitor `sendkey`), judged by `tests/r12/judge_s1.py`: plain, `/W`, `/W` with brackets, unknown switch (`/X`, `/W /X`), empty directory, quoted long directory with `/W` before and after the path and with `/P`, a listing of exactly one page, one line more, pause, SPACE, ENTER, Q, Esc, Ctrl+C, wide + pause | guest executed |
| `tests/r12/seq-s1-edit.txt` on QEMU: Up/Down history, insert in the middle, Home/Delete, End/Backspace, Ctrl+U, Esc, Ctrl+K, Ctrl+A, Ctrl+C, quote recall, 170-character overflow (stops at 159), a 101-character line edited at Home/End/Delete/Backspace over two rows, the same at the bottom of the screen (scroll) | guest executed |
| `tests/host/r12/test_keys.c` (decoder and console treatment of the key codes, strict C89) | host build |
