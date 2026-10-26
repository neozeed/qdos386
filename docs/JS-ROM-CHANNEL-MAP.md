# JS ROM Channel Map (TRAP #2 and TRAP #3)

Source: `js-rom-gas-r2/src/system_asm.s` (the commented GAS conversion of the
JS ROM, the reference for this audit). Line numbers refer to that file.
Everything in this document describes the **original 68000 ABI**. The native
i386 ABI that R4 implements is in `docs/R4-CHANNEL-ABI.md` and is not the same
thing. Where the two are compared, the comparison is labelled.

Status: the output operation codes, the register convention, the error codes
and the channel lookup are resolved from source. Where the ROM's own comment
disagrees with its code, the code is recorded and the disagreement is noted.

## 1. Entry points

| Routine | Line | Role |
|---|---|---|
| `ltrap2` | 427 | TRAP #2 entry: sets A5/A6, continues at `l032a2` |
| `ltrap3` | 430 | TRAP #3 entry: sets A5/A6, continues at `l0337c` |
| `l00336` | 437 | common A5/A6 setup for trap 2 and trap 3 |

TRAP #2 is channel management: open, close, format, delete.
TRAP #3 is channel I/O. The I/O operation number is in D0 (only the low seven
bits are used, `andi.l #0x7f,d0`, line 4841).

## 2. TRAP #2 dispatch (`l032a2`, line 4747)

D0 selects the operation:

| D0 | Operation | Routine |
|---|---|---|
| 1 | open | `l032d0` (line 4765) |
| 2 | close | `l03352` (line 4820) |
| 3 | format | `l036b6` |
| 4 | delete | `l03552` |
| other | bad parameter, D0 = -15 | `l003a6` |

Open (`l032d0`):

- Finds a free channel table entry between `SV.CHBAS` and `SV.CHTOP`
  (line 4773). A free entry has its first byte negative (`tst.b (a3)`,
  `blt`). None free: D0 = -6.
- Walks the driver list `SV.DRLIST` (line 4789). For each driver it calls the
  driver's open entry at driver+8 (`movea.l 0x0008(a2),a4`), with A3 = driver
  base minus 0x18. D0 = -7 (`0xfff9`) means "name not known to this driver":
  the walk continues. Any other error stops the walk.
- On success the channel block is filled (`l0331e`, line 4797):
  `+0x04` driver pointer, `+0x08` owning job id (D1), `+0x0C` table slot
  address, `+0x10` CH.TAG word, `+0x12` CH.STAT, `+0x14` CH.JOBW.
- The channel id returned in A0 is `(tag << 16) | index`, where `index` is the
  table offset divided by 4 (`swap`, `move.w a3,d2`, `lsr.w #2,d2`, lines
  4797–4812). The tag comes from `SV.CHTAG` and is incremented on every open.

Close (`l03352`, line 4820):

- Looks up the channel (`l03476`, see §5), then calls the driver's close entry
  at driver+0x0C (`0x000c(a4)`, line 4826), then marks the table slot free with
  `st (a0)` (line 4831).
- The ROM does not check which job owns the channel. R4 does (see R4 ABI).

## 3. TRAP #3 I/O (`l0337c`, line 4836)

Register convention on entry (from the code, not from a comment):

| Register | Meaning |
|---|---|
| A0 | channel id on entry, channel block address after lookup |
| D0 | I/O operation code (low seven bits used) |
| D1 | byte to send (op 5), or the initial count (op 7) |
| D2 | length in bytes (op 7, word) |
| A1 | buffer address (op 7) |

Result: D0 = 0 on success, -1 "not complete" (the call would block), or a
negative error. For op 7, D1 is the number of bytes transferred and A1 is
advanced past them (`l00eee`, line 1789).

Steps (lines 4836–4900):

1. `l03476` looks up the channel. Failure returns D0 = -6, "channel not open".
2. `tas 0x12(a0)` test-and-sets CH.STAT. If the channel was already busy the
   job is put on hold (`l03414`, line 4884). That is the blocking path. R4 has
   no blocking I/O, so this path does not exist in R4.
3. For operation codes 2, 3, 6, 7 and 0x46–0x49 the routine clears D1 first
   (`l033ae`, line 4851). This affects only the input side and the
   trap-4 bookkeeping, and R4 does not depend on it, so it is not traced
   further.
4. CH.ACTN (`+0x13`) is set to the operation code, and the driver I/O entry is
   called at driver+4 (`0x0004(a4)`) with A3 = driver base minus 0x18
   (lines 4858–4862). Per-operation work happens there (§3.1).

### 3.1 Driver I/O dispatch for CON / SCR (`l00e76`, line 1701)

The console channel driver is `CON` (name at line 5639 and 1585). Its I/O entry
is `l00e76`:

- If the screen is frozen (`0x33(a6)` nonzero): D0 = -1, not complete.
- Codes above 7 go to the screen-printing trap dispatch `l01992` (line 2157),
  which implements the window and cursor operations 0x05 and 0x09–0x36.
- Codes 0–7 go through the byte table `l00eb8` (line 1727):

| Code | Table entry | Routine | Behaviour |
|---|---|---|---|
| 0 | `l00ec0` | `l0380a` | IO.QTEST: test pending keyboard input |
| 1 | `l00ec6` | `l0385e` | put a byte into the keyboard queue (input side) |
| 2 | `l00f54` | | input (line read) |
| 3 | `l00eca` | | edit line: copy from keyboard queue, input side |
| 4 | `l00f5a` | | line editor, input side |
| **5** | `l00f00` via `l00efa` | `l00f00` (line 1770) | **IO.SBYTE: output one byte in D1 to the screen** |
| 6 | `l00eec` | | **not used**: D0 = -15 (bad parameter) |
| **7** | `l00ede` | `l00f00` per byte | **IO.SSTRG: output D2 bytes from A1 to the screen** |

`l00efa` sets D0 = 5 and falls into `l01992`, which for code 5 calls
`l01a4c` (line 2251) to print the character in D1 at the cursor. So code 5 is
the single-character output path, and it prints through the screen driver.

### 3.2 String output, code 7 (`l00ede`, line 1753)

```
loop:   moveq #0,d0
        cmp.w d4,d5          ; done when count (D4) reaches length (D5)
        bls   done
        move.b (a4)+,d1      ; next byte from the buffer
        bsr   l00f00         ; output one byte (same path as code 5)
        addq.w #1,d4
        bra   loop
done:   move.w d4,d1         ; D1 = number of bytes transferred
        movea.l a4,a1        ; A1 = buffer address after the transfer
```

Observations:

- The per-byte result of `l00f00` is not tested in the loop. The routine
  returns D0 = 0 on exit, as the code sets D0 before each test.
- The ROM's comment on this table entry reads "put in queue". The code
  writes to the screen through `l00f00`, not to the keyboard queue. The code
  is authoritative and is recorded here.
- `l00f00` interprets byte 0x0A (LF) as a new line, and it handles cursor
  wrap and window scrolling. R4 writes bytes raw to the backend and does not
  reproduce cursor handling. This is a deliberate difference, listed in the
  R4 limits.

## 4. Error codes (from the code)

| Code | Where | Meaning |
|---|---|---|
| -1 | `l00e76`, `l0337c` | not complete (would block, or screen frozen) |
| -3 | `l0329e` | out of memory (open) |
| -6 | `l03444` | channel not open (no entry, or tag mismatch) |
| -7 | driver open walk | name not known to this driver (continue) |
| -15 | `l00eec`, `l003a6` | bad parameter (unused operation, unknown trap 2 code) |

## 5. Channel lookup (`l03444`, line 4909)

Input: the channel id in A0 (`index | tag << 16`). Steps:

1. The index (low word of A0) must be no greater than `SV.CHMAX`.
2. `index * 4` is added to `SV.CHBAS`. The table entry's first byte must not be
   negative (negative means free).
3. The entry holds the channel block address. Its `CH.TAG` word (`+0x10`) must
   equal the tag (high word of the id).

Any failure gives D0 = -6 and does not touch the channel. A stale id therefore
fails with -6 when the slot is free, and also when the slot has been reused
with a new tag. This is the behaviour R4 reproduces with its own error code
(see R4 ABI, `-12`).

## 6. Comparison with R4 (native i386 ABI)

| Item | JS ROM (68000) | R4 (native) |
|---|---|---|
| Entry | TRAP #2 / TRAP #3 | `int $0x30`, codes 0x14–0x17 |
| Operation | D0 | EAX |
| Channel id | 32-bit `tag << 16 | index` | same shape, own tag counter |
| Byte output | TRAP #3 D0=5, byte in D1 | `QT_CH_PUTC` (0x17), byte in ECX |
| String output | TRAP #3 D0=7, A1 buffer, D2 length | `QT_CH_WRITE` (0x16), ECX buffer, EDX length |
| Count result | D1 | EAX (bytes written) |
| Not-open error | -6 | -12 (invalid or stale id) |
| Owner check | none | owner must equal the calling job (-13) |
| Blocking | CH.STAT / -1 | not in R4 |
| Cursor and LF handling | screen driver | none; bytes are raw |
| Keyboard, files | yes | not in R4 |

## 7. Not traced in this pass

- The trap 2 format (3) and delete (4) routines, which R4 does not use.
- The driver table layout beyond the offsets above. R4 uses its own
  driver structure, so the exact layout does not affect R4.
- The screen printing internals (`l01a4c` and `l02840`), which R4 does not use.
