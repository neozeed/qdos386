# R11M virtual text consoles

Applies to kernels built with `-DQDOS_VCON` (which requires `-DQDOS_FS_WRITE`; `make r11m-kernels`). Kernels built without it are byte-identical to
R11L (checked by `scripts/r11l-default-check.sh`). No graphics, no VBE, no terminal emulation: the consoles are 80x25 text cell arrays.

## 1. Model

* `VCON_MAX` = 4 consoles, numbered 0..3, each 80 columns x 25 rows of cells (character + attribute), with its own cursor (position,
  visible flag), current attribute (default 0x07), scroll region, input queue (`VCON_QSIZE` 128, 127 usable), cooked-line buffer, mode, EOF flag
  and one input waiter.
* One console is the **foreground** console (console 0 at boot). The display shows it and the keyboard feeds it. Output to a background console
  only changes its buffer; when it becomes foreground the whole buffer is redrawn (`vdisp_load`) and the cursor restored.
* Output rules: `\r` column 0; `\n` column 0 and next row (scrolls within the scroll region only when the cursor is on its last row); `\b` one column
  left (no erase); `\t` next multiple of 8; other control bytes and 0x7F are ignored; printable bytes are stored with the current attribute. A write to the
  last column wraps lazily (the wrap happens when the next printable byte arrives). No escape sequences are interpreted.
* A CON channel is **bound to a console when it is opened**: the console of the opening job. Children inherit the creator's console (or the console the
  creator chose with `qd_con_child`/`START /CON:n`). The shell owns console 0.
* **Reclaim policy.** When a job ends and it was the last user of the foreground console, the focus returns to console 0. Background consoles keep their
  buffers (so `CONDUMP n` shows what a finished job printed).

## 2. Diagnostics stay separate

Kernel messages and console 0 text go to the diagnostic stream (PC: port 0x402, optionally serial 0x3F8; i386VM: the harness byte port) exactly as before. The
mirror mode of program output is selectable (`DIAG n` in the shell, `qd_con_diag`): 0 = console 0 only (default, the legacy transcript), 1 = kernel
messages only, 2 = all consoles, each line of consoles 1..3 starting with `[Cn] ` (a change of console always starts a new line). Kernel text (`vcon_kputc`) is
always sent to the stream.

## 3. Keyboard and focus

* PC (i8042): Alt+F1..Alt+F4 or Alt+1..Alt+4 select console 0..3. Both Alt keys are tracked (right Alt = E0 38). The key is consumed by the kernel: nothing
  reaches an application (checked on QEMU: no stray characters in either console).
* Polled backends (i386VM harness stream): the two bytes ESC followed by `1`..`4` select console 0..3 (a lone ESC is passed on when anything else follows).
  Tiny386 `-term` turns ESC+`n` into Alt+`n`, so the same key reaches the guest as the PC hotkey.
* Shell fallback on every emulator: `CON n`; `CON` alone reports the shell's console and the visible one. A program may call `qd_con_focus`.
* Input is delivered to the foreground console's queue only. A polled backend is throttled when the queue has fewer than 8 free bytes, so a long pasted stream
  is not lost (150 lines through console 1: 150 received, no gap).

## 4. Input modes

`qd_con_mode`: `0` raw (default, legacy): bytes are delivered as typed; the program echoes. `QD_CON_ECHO`: the kernel shows what is typed (CR/LF as CR LF,
BS/DEL as erase). `QD_CON_COOKED`: a line is delivered only when Enter is pressed (the line, then `\n`); BS/DEL and ^U edit it; lines longer than 120 are cut;
the half-typed line is discarded when the mode changes. A queue that cannot take a whole line drops it (counted).

## 5. Waiting

`QT_CON_WAIT` (0x1C) sleeps the caller until **its own console** has input or EOF. One waiter per console: a second waiter on the same console gets `-50`
(`QD_ERR_CONBUSY`) and must back off (`qd_con_getc` does). Waiters on different consoles are independent (the shell on console 0 and a program on console 1
wait at the same time). The wait check and the sleep happen inside a trap (interrupts off), and the keyboard handler only runs between traps, so a byte that
arrives while a job is going to sleep wakes it; a wake never schedules from the handler (`vcon_wake_job` releases the hold only). EOF of the input backend
marks every console, so every waiter wakes and `qd_con_getc` returns `QD_EOF`.

## 6. libqdos API (`sdk/include/qdos386.h`, `sdk/lib/qdoscon.c`)

All take a CON channel (`qd_con_channel()` of libc, or `qd_open("CON")`). **Rows and columns are 0-based**, except `qd_con_gotoxy1` (1-based, ANSI order).

| Call | Meaning |
|---|---|
| `qd_con_goto(ch,row,col)` | cursor (range `0..24`, `0..79`, else `QD_ERR_RANGE` -14) |
| `qd_con_gotoxy1(ch,row,col)` | the same, 1-based |
| `qd_con_clear(ch,what)` | `QD_CLR_ALL` (cursor home), `EOL`, `LINE`, `EOS` |
| `qd_con_attr(ch,attr)` | set the attribute for new text (`QD_ATTR(fg,bg)`); returns the old one |
| `qd_con_size(ch,&rows,&cols)` | 25 x 80 |
| `qd_con_cursor(ch,&row,&col)` | position; returns 1 if the cursor is visible |
| `qd_con_show_cursor(ch,show)` | hide/show (hardware cursor only for the visible console) |
| `qd_con_mode(ch,flags)` | see section 4; returns the old flags |
| `qd_con_region(ch,top,bottom)` | scroll region (inclusive); 0..24 restores |
| `qd_con_number(ch)`, `qd_con_is_foreground(ch)`, `qd_con_focus(ch,n)` | identity and focus |
| `qd_con_child(ch,n)` | the next job this job creates goes to console `n` (-1 clears) |
| `qd_con_snap(ch,con,row,buf,size)` | text of one row of a console (trailing blanks trimmed, control bytes shown as `.`) |
| `qd_con_getc(ch)` | blocking one-byte read (kernel console wait) |
| `qd_con_diag(ch,mode)` | diagnostic mirror mode (-1 reads) |

Kernel interface: `CH_CTL` (0x2B) on a CON channel with ops `0x10..0x1A` (`kernel/channel.h`); `GOTO` takes `row | col << 8`, `REGION` `top | bottom << 8`.
`SNAP` takes a pointer to an 88-byte request, range-checked against the arena. Arguments are validated twice (library, kernel); a kernel without VCON answers
`-15` (not supported) for all of them.

## 7. Shell commands (R11M shell)

`CON [n]`, `CONDUMP n` (prints the text of console n, `|`-prefixed rows, ends with `CONDUMP END`), `DIAG [0|1|2]`, `SLEEP ticks`, `START path [args] [/CON:n]`
(runs a slot-1 program without waiting and prints `job <id> started on console <n>`; Stage 3 adds `PS`, `WAIT`, `KILL`).

## 8. Verification

| Evidence | Class |
|---|---|
| `tests/host/r11m/test_vcon.c` (104 checks: output rules, scrolling and regions, clear, cursor, cooked editing, queue/EOF, waiters, session tables, diag tags; strict C89 `-Werror`) | host build |
| `CONT1.AOU`: 45 checks of the whole API through `qd_con_snap` | guest executed (i386VM, QEMU) |
| `COUNTER.AOU` on console 1 while the shell works on console 0; Alt+F2/Alt+F1 on QEMU VGA (screen capture shows the counter only on console 1, the shell only on console 0, no leaked keys) | guest executed, QEMU keyboard path |
| `CONINP.AOU`: cooked + echo input to console 1 and `LEAK` typed on console 0 reaching only the shell; BS editing (`abx<BS>c` -> `abc`) | guest executed (QEMU, Tiny386 via Alt+2/Alt+1, i386VM via ESC) |
| 150 lines streamed to console 1 on i386VM: 150 `GOT` lines, no gap | guest executed |
| default images byte-identical (six hashes) | host build |

## 9. Limits

No escape-sequence terminal, no scrollback, no per-console colour palette beyond the 16/8 VGA attributes (bit 7 blinks as on VGA), 4 consoles only, one waiter
per console, the diagnostic stream is shared (tag lines with `DIAG 2`). Two jobs sharing a console share its input queue and cursor. Not memory protection:
consoles isolate text and input, not memory.
